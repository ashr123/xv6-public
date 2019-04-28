#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void) {
	return fork();
}

void sys_exit(void) {
	exit();
}

int sys_wait(void) {
	return wait();
}

int sys_kill(void) {
	int pid;

	if (argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int sys_getpid(void) {
	return kthread_id();
}

int sys_sbrk(void) {
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

int sys_sleep(void) {
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (myproc()->killed) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void) {
	acquire(&tickslock);
	uint xticks = ticks;
	release(&tickslock);
	return xticks;
}

int sys_kthread_create(void) {
	void (*start_func)();
	void *stack;
	if (argptr(0, (void **) &start_func, 0) < 0 || argptr(1, &stack, 0) < 0)
		return -1;
	return kthread_create(start_func, stack);
}

int sys_kthread_id(void) // Added
{
	return kthread_id();
}

void sys_kthread_exit(void) // Added
{
	kthread_exit();
}

int sys_kthread_join(void) // Added
{
	int tid_sleep;
	if (argint(0, &tid_sleep) < 0)
		return -1;
	return kthread_join(tid_sleep);
}

int sys_kthread_mutex_alloc(void) // Added
{
	return kthread_mutex_alloc();
}

int sys_kthread_mutex_dealloc(void) // Added
{
	int pid;
	if (argint(0, &pid) < 0)
		return -1;
	return kthread_mutex_dealloc(pid);
}

int sys_kthread_mutex_lock(void) // Added
{
	int pid;
	if (argint(0, &pid) < 0)
		return -1;
	return kthread_mutex_lock(pid);
}

int sys_kthread_mutex_unlock(void) // Added
{
	int pid;
	if (argint(0, &pid) < 0)
		return -1;
	return kthread_mutex_unlock(pid);
}
