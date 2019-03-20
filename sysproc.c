#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
	return fork();
}

//new
int sys_detach(void)
{
	int pid; //child id
	argint(0, &pid);
	return detach(pid);
}

void sys_priority(void)
{
	int prio;
	argint(0, &prio);
	priority(prio);
}

void sys_policy(void)
{
	int poly;
	argint(0, &poly);
	policy(poly);
}

int sys_wait_stat(void)
{
	int *status;
	struct perf *performance;

	argptr(0, (void *)&status, sizeof(int));
	argptr(1, (void *)&performance, sizeof(struct perf));

	return wait_stat(status, performance);
}

void sys_exit(void)
{
	int status;
	argint(0, &status);
	exit(status);
}

int sys_wait(void)
{
	int *status;
	argptr(0, (void *)&status, sizeof(int));
	return wait(status);
}

int sys_kill(void)
{
	int pid;

	if (argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int sys_getpid(void)
{
	return myproc()->pid;
}

int sys_sbrk(void)
{
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

int sys_sleep(void)
{
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n)
	{
		if (myproc()->killed)
		{
			release(&tickslock);
			return -1;
		}
		sleep((void *)&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}
