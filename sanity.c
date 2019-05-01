האחסון שלי
// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "kthread.h"
#include "tournament_tree.h"

#define MAX_STACK_SIZE 4000

int counter = 0;
int mid;
int help;
trnmnt_tree *tree;

void
test2_1(void)
{
	char *cmd = "echo";
	int pid;
	char *cmds[2];
	cmds[0] = "echo";
	cmds[1] = "echo work just fine!";
	if ((pid = fork()) == 0)
	{
		exec(cmd, cmds);
	}
	if (pid > 0)
	{
		wait();
		printf(1, "child finish echo!\n");
		exit();
	} else
	{
		printf(1, "fork failed\n");
	}
}

void
thread_task22()
{
	int tid = kthread_id();
	sleep(100 * tid);
	printf(1, "thread %d entering\n", tid);
	printf(1, "thread %d exiting\n", tid);
	kthread_exit();
}


void
test2_2(void)
{
	void *stack;
	int threads[8];
	for (int i = 0; i < 8; i++)
	{
		stack = malloc(MAX_STACK_SIZE);
		threads[i] = kthread_create(&thread_task22, stack);
		if (threads[i] < 0)
			printf(1, "kthread_create failed\n");
	}


	for (int i = 0; i < 8; i++)
		if (kthread_join(threads[i]) < 0)
			printf(1, "join failed\n");


	printf(1, "finish join 8 threads\n");
}


void
thread_task31(void)
{
	if (kthread_mutex_lock(mid) < 0)
	{
		printf(1, "lock failed!\n");
	}

	counter++;
	if (kthread_mutex_unlock(mid) < 0)
		printf(1, "unlock failed! thread 2\n");
	kthread_exit();
}

void
test3_1(void)
{

	mid = kthread_mutex_alloc();
	if (kthread_mutex_lock(mid) < 0)
		printf(1, "lock failed!\n");
	printf(1, "lock %d !\n", mid);
	void *stack;
	if ((stack = malloc(MAX_STACK_SIZE)) < 0)
		printf(1, "malloc(MAX_STACK_SIZE) failed!\n");
	int thread = kthread_create(&thread_task31, stack);
	if (thread == -1)
		printf(1, "test3_1: kthread_create failed!");
	printf(1, "counter = %d , need to be 0\n", counter);
	// sleep(1000);
	if (kthread_mutex_unlock(mid) < 0)
		printf(1, "unlock failed! main\n");
	kthread_join(thread);
	printf(1, "counter = %d , need to be 1\n", counter);
	printf(1, "unlock %d !\n", mid);

}

void
thread_task32(void)
{
	if (trnmnt_tree_acquire(tree, kthread_id() - help - 1) < 0)
		printf(1, "aquire failed\n");
	counter++;
	printf(1, "thread %d made counter = %d\n", kthread_id() - help, counter);
	if (trnmnt_tree_release(tree, kthread_id() - help - 1) < 0)
		printf(1, "release failed\n");
	kthread_exit();

}

void
test3_2(void)
{
	tree = trnmnt_tree_alloc(3);
	help = kthread_id();
	void *stack;
	int tids[10];
	for (int i = 0; i < 8; i++)
	{
		stack = malloc(MAX_STACK_SIZE);
		tids[i] = kthread_create(&thread_task32, stack);
	}
	for (int i = 0; i < 8; i++)
	{
		kthread_join(tids[i]);
	}
	if (trnmnt_tree_dealloc(tree) < 0)
		printf(1, "dealloc failed\n");
	printf(1, "joined eveyone and the counter is %d when it should be 8", counter);
}

int
main(void)
{
	// printf(1,"test 2.1 :\n");  
	// test2_1();
	// printf(1,"test 2.2 :\n");  
	// test2_2();
	printf(1, "test 3.1 :\n");
	test3_1();
	// printf(1,"test 3.2 :\n");  
	// test3_2();
	exit();
}