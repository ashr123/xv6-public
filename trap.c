#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
	int i;

	for (i = 0; i < 256; i++)
	SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
	SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

	initlock(&tickslock, "time");
}

void
idtinit(void)
{
	lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
	struct proc *p = myproc();
	if (tf->trapno == T_SYSCALL)
	{
		if (p->killed)
			exit();
		p->tf = tf;
		syscall();
		if (p->killed)
			exit();
		return;
	}
	/////added
	pte_t * pte;
	/////////

	switch (tf->trapno)
	{
		case T_IRQ0 + IRQ_TIMER:
			if (cpuid() == 0)
			{
				acquire(&tickslock);
				ticks++;
				wakeup(&ticks);
				release(&tickslock);
			}
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_IDE:
			ideintr();
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_IDE + 1:
			// Bochs generates spurious IDE1 interrupts.
			break;
		case T_IRQ0 + IRQ_KBD:
			kbdintr();
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_COM1:
			uartintr();
			lapiceoi();
			break;
		case T_IRQ0 + 7:
		case T_IRQ0 + IRQ_SPURIOUS:
			cprintf("cpu%d: spurious interrupt at %x:%x\n",
			        cpuid(), tf->cs, tf->eip);
			lapiceoi();
			break;

		case T_PGFLT:
		 	p->faultCounter++;
			//cprintf("\nunexp99999777777777777777777999999999ected");
			pte = walkpgdir(p->pgdir,(void *)rcr2(),0);
			if((*pte & PTE_PM) && !(*pte & PTE_W))
				tf->trapno =T_GPFLT;
			if (p != 0 && (tf->cs & 3) == DPL_USER && isPageInFile(rcr2(), p->pgdir))
				if (getPageFromFile(rcr2()))
					break;

			//PAGEBREAK: 13
		default:
			if (p == 0 || (tf->cs & 3) == 0)
			{
				// In kernel, it must be our mistake.
				cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
				        tf->trapno, cpuid(), tf->eip, rcr2());
				panic("trap");
			}
			// In user space, assume process misbehaved.
			cprintf("pid %d %s: trap %d err %d on cpu %d "
			        "eip 0x%x addr 0x%x--kill proc\n",
			        p->pid, p->name, tf->trapno,
			        tf->err, cpuid(), tf->eip, rcr2());
			p->killed = 1;
	}

	// Force process exit if it has been killed and is in user space.
	// (If it is still executing in the kernel, let it keep running
	// until it gets to the regular system call return.)
	if (p && p->killed && (tf->cs & 3) == DPL_USER)
		exit();

	// Force process to give up CPU on clock tick.
	// If interrupts were on while locks held, would need to check nlock.
	if (p && p->state == RUNNING &&
	    tf->trapno == T_IRQ0 + IRQ_TIMER)
		yield();

	// Check if the process has been killed since we yielded
	if (p && p->killed && (tf->cs & 3) == DPL_USER)
		exit();
}
