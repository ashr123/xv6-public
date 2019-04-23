#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct
{
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nexttid = 1;
int nextpid = 1;

extern void forkret(void);

extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int cpuid()
{
	return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
	if (readeflags() & FL_IF)
		panic("mycpu called with interrupts enabled\n");

	int apicid = apicid = lapicid();
	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (int i = 0; i < ncpu; ++i)
	{
		if (cpus[i].apicid == apicid)
			return &cpus[i];
	}
	panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
	pushcli();
	struct proc *p = mycpu()->proc;
	popcli();
	return p;
}

struct thread *mythread(void)
{
	pushcli();
	struct thread *t = mycpu()->thread;
	popcli();
	return t;
}

static void
allocthread(struct proc *p)
{
	struct thread *t;
	for (t = p->threads; t < &p->threads[NTHREAD]; t++)
		if (t->state == THREAD_UNUSED)
			break;

	if (t == &p->threads[NTHREAD])
		panic("allocthread failed");

	t->tid = nexttid++;

	t->owner = p;

	t->killed = 0;
	// Allocate kernel stack.
	if ((t->kstack = kalloc()) == 0) //????
	{
		p->state = PROC_UNUSED;
	}
	char *sp = sp = t->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *t->tf;
	t->tf = (struct trapframe *) sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint *) sp = (uint) trapret;

	sp -= sizeof *t->context;
	t->context = (struct context *) sp;
	memset(t->context, 0, sizeof *t->context);
	t->context->eip = (uint) forkret;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void)
{
	acquire(&ptable.lock);
	struct proc *p;
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == PROC_UNUSED)
			goto found;

	release(&ptable.lock);
	return 0;

	found:
	p->state = EMBRYO;
	p->pid = nextpid++;

	// Allocate kernel stack.

	struct thread *th;
	for (th = p->threads; th < &p->threads[NTHREAD]; th++)
		th->state = THREAD_UNUSED;
	allocthread(p);
	release(&ptable.lock);
	return p;
}

//PAGEBREAK: 32
// Set up first user process.
void userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];
	p = allocproc();

	initproc = p;
	if ((p->pgdir = setupkvm()) == 0)
		panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);

	p->sz = PGSIZE;
	//allocate first thread tf in proc
	struct trapframe *thread_tf = p->threads[0].tf;
	memset(thread_tf, 0, sizeof(*thread_tf));
	thread_tf->cs = (SEG_UCODE << 3) | DPL_USER;
	thread_tf->ds = (SEG_UDATA << 3) | DPL_USER;
	thread_tf->es = thread_tf->ds;
	thread_tf->ss = thread_tf->ds;
	thread_tf->eflags = FL_IF;
	thread_tf->esp = PGSIZE;
	thread_tf->eip = 0; // beginning of initcode.S
	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	// this assignment to p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->threads[0].state = RUNNABLE;

	release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
	uint sz;
	acquire(&ptable.lock); //????
	struct proc *curproc = myproc();

	sz = curproc->sz;
	if (n > 0)
	{
		if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
		{
			release(&ptable.lock);
			return -1;
		}
	} else if (n < 0)
	{
		if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
		{
			release(&ptable.lock);
			return -1;
		}
	}
	curproc->sz = sz;
	switchuvm(mythread());
	release(&ptable.lock);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void)
{
	struct proc *np;
	struct proc *curproc = myproc();
	//struct thread *curthread = mythread();

	// Allocate process.
	if ((np = allocproc()) == 0)
	{
		return -1;
	}

	// Copy process state from proc.
	if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
	{
		kfree(np->threads[0].kstack);
		np->threads[0].kstack = 0;
		np->state = PROC_UNUSED;
		return -1;
	}
	np->sz = curproc->sz;
	np->parent = curproc;
	*np->threads[0].tf = *curproc->threads[0].tf;

	// Clear %eax so that fork returns 0 in the child.
	np->threads[0].tf->eax = 0;

	for (int i = 0; i < NOFILE; i++)
		if (curproc->ofile[i])
			np->ofile[i] = filedup(curproc->ofile[i]);
	np->cwd = idup(curproc->cwd);

	safestrcpy(np->name, curproc->name, sizeof(curproc->name));

	int pid = np->pid;

	acquire(&ptable.lock);

	np->threads[0].state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

//void lockptable()
//{
//	acquire(&ptable.lock);
//}
//
//void unlockptable()
//{
//	release(&ptable.lock);
//}

void close_thread(struct thread *t)
{
	kfree(t->kstack);
	t->kstack = 0;
	t->tid = 0;
	t->killed = 0;
	t->state = THREAD_UNUSED;
}

void close_proc(struct proc *curproc)
{
	// Close all open files.
	for (int fd = 0; fd < NOFILE; fd++)
	{
		if (curproc->ofile[fd])
		{
			fileclose(curproc->ofile[fd]);
			curproc->ofile[fd] = 0;
		}
	}

	begin_op();
	iput(curproc->cwd);
	end_op();
	curproc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	{
		if (p->parent == curproc)
		{
			p->parent = initproc;
			if (p->state == PROC_ZOMBIE)
				wakeup1(initproc);
		}
	}

	// Jump into the scheduler, never to return.
	curproc->state = PROC_ZOMBIE;
}

void exit_thread(void)
{

	struct proc *curproc = myproc();

	struct thread *curthread = mythread();
	struct thread *t;

	acquire(&ptable.lock);
	for (t = curproc->threads; t < &curproc->threads[NTHREAD]; t++)
		if (t->tid != curthread->tid && (t->state == RUNNABLE || t->state == RUNNING || t->state == SLEEPING))
		{
			//this thread is not the last alive--make m yself zombie and ret to sched
			curthread->state = THREAD_ZOMBIE;
			sched();
			panic("zombie exit_thread");
		}

	release(&ptable.lock);
	//THREAD IS THE LAST  IN PROC
	close_proc(curproc);

	curthread->state = THREAD_ZOMBIE;
	sched();
	panic("zombie exit_thread");
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
	struct proc *curproc = myproc();
	struct thread *t;
	acquire(&ptable.lock);
	if (curproc == initproc)
		panic("init exiting");
	//make all threads sonskill themself
	for (t = curproc->threads; t < &curproc->threads[NTHREAD]; t++)
	{
		if (t->state != THREAD_UNUSED)
			t->killed = 1;
		if (t->state == SLEEPING)
			t->state = RUNNABLE;
	}
	curproc->killed = 1;
	release(&ptable.lock);
	exit_thread();
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void)
{
	struct proc *p;
	struct thread *t;
	int havekids, pid;
	struct proc *curproc = myproc();

	acquire(&ptable.lock);
	for (;;)
	{
		// Scan through table looking for exited children.
		havekids = 0;
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		{
			if (p->parent != curproc)
				continue;
			havekids = 1;
			if (p->state == PROC_ZOMBIE)
			{
				// Found one.
				pid = p->pid;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = PROC_UNUSED;
				for (t = curproc->threads; t < &curproc->threads[NTHREAD]; t++)
				{
					if (t->state == THREAD_ZOMBIE)
						close_thread(t);
				}

				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if (!havekids || curproc->killed)
		{
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock); //DOC: wait-sleep
	}
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
	//struct thread *t;
	struct proc *p;
	struct cpu *c = mycpu();
	c->proc = 0;
	c->thread = 0;

	for (;;)
	{
		// Enable interrupts on this processor.
		sti();

		// Loop over process table looking for process to run.
		acquire(&ptable.lock);
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		{
			if (p->state != EMBRYO) //need to be used-like
				continue;

			// Switch to chosen process.  It is the process's job
			// to release ptable.lock and then reacquire it
			// before jumping back to us.

			//get RUNNABLE thread from p

			for (struct thread *t = p->threads; t < &p->threads[NTHREAD]; t++)
			{
				if (t->state != RUNNABLE)
					continue;
				c->thread = t;
				c->proc = p;
				switchuvm(t);
				t->state = RUNNING;
				swtch(&(c->scheduler), t->context);
				switchkvm();


				c->proc = 0;
				c->thread = 0;
			}
		}
		release(&ptable.lock);
	}
}

// nter schedEuler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
	int intena;
	//struct proc *p = myproc();

	if (!holding(&ptable.lock))
		panic("sched ptable.lock");
	if (mycpu()->ncli != 1)
		panic("sched locks");
	if (mythread()->state == RUNNING)
		panic("sched running");
	if (readeflags() & FL_IF)
		panic("sched interruptible");
	intena = mycpu()->intena;
	swtch(&mythread()->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
	acquire(&ptable.lock); //DOC: yieldlock
	mythread()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
	static int first = 1;
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first)
	{
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		iinit(ROOTDEV);
		initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
	struct thread *t = mythread();

	if (t == 0)
		panic("sleep");

	if (lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if (lk != &ptable.lock)
	{                           //DOC: sleeplock0
		acquire(&ptable.lock); //DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	t->chan = chan;
	t->state = SLEEPING;

	sched();

	// Tidy up.
	t->chan = 0;

	// Reacquire original lock.
	if (lk != &ptable.lock)
	{ //DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		for (struct thread *t = p->threads; t < &p->threads[NTHREAD]; t++)
			if (t->state == SLEEPING && t->chan == chan)
				t->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
	acquire(&ptable.lock);
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	{
		if (p->pid == pid)
		{
			p->killed = 1;
			// Wake threads from sleep if necessary.
			for (struct thread *t = p->threads; t < &p->threads[NTHREAD]; t++)
			{
				t->killed = 1;
				if (t->state == SLEEPING)
					t->state = RUNNABLE;
			}

			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

int kthread_create(void (*start_func)(), void *stack){

}


int kthread_id(){
	int id = mythread()->tid;
	if(id == 0)
		return -1;
	return id;
}

void kthread_exit(){

	if(myproc()==initproc){
      	panic("exit from init proc");
	}
	//there may bethreads sleeping on me
    wakeup(mythread());
	exit_thread();
}

	//PAGEBREAK: 36
	// Print a process listing to console.  For debugging.
	// Runs when user types ^P on console.
	// No lock to avoid wedging a stuck machine further.
	/*
void procdump(void)
{
	static char *states[] = {
		[UNUSEDP] "unused",
		[EMBRYO] "embryo",
		[USED] "used ",
		[ZOMBIEP] "zombie"};
	//int i;
	struct proc *p;
	char *state;
	//uint pc[10];

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
	{
		if (p->state == UNUSEDP)
			continue;
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s", p->pid, state, p->name);
		// if (p->state == SLEEPING)
		// {
		// 	getcallerpcs((uint *)p->context->ebp + 2, pc);
		// 	for (i = 0; i < 10 && pc[i] != 0; i++)
		// 		cprintf(" %p", pc[i]);
		// }
		cprintf("\n");
	}
}
*/