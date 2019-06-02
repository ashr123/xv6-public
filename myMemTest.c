#include "types.h"
#include "stat.h"
#include "user.h"
#define ONEKBYTE 1024
#define TEWKBYTE 2048
#define THREEKBYTE 3072
#define PGSIZE 4096
int testingPmallocAndPfree();
int pfree_foults(); // need to do
int file_test();	// need to do
int testFork();

int main(int argc, char *argv[])
{
#ifdef SCFIFO
	printf(1, "\n SCFIFO \n");
#endif

#ifdef LIFO
	printf(1, "\n LIFO \n");
#endif
	// testingPmallocAndPfree();
	// testFork();
	// printf(1, "in main, after fork swap\n");
	// pfree_foults();

	file_test();
	exit();
}

int pfree_foults()
{
	printf(1, "\n----------------------TEST STARTED: pfree_foults------ \n");
	if (fork() == 0)
	{

		void *mem1 = malloc(1);
		int result1 = pfree(mem1);
		if (result1 == -1)
		{
			printf(1, "pfree_foults: success\n");
		}
		else
		{
			printf(1, "ERROR pfree_foults:fail! pfree without protect\n");
		}

		void *mem2 = pmalloc();
		int result2 = pfree(mem2);
		if (result2 == -1)
		{
			printf(1, "pfree_foults: success\n");
		}
		else
		{
			printf(1, "-------ERROR pfree_foults:fail! pfree without protect\n");
		}

		exit();
	}
	else
	{
		sleep(6);
		wait();
	}
	printf(1, "TEST ENDED: pfree_foults\n");
	return 0;
}

int testingPmallocAndPfree()
{
	if (fork() == 0)
	{
		printf(1, "--------------start pmalloc and pfree test--------------\n");
		void *pmem = pmalloc();
		int tryToProtect = protect_page(pmem);
		if (tryToProtect < 0)
		{
			printf(1, "--------------testingPmallocAndPfree FAILD: protect_page failed--------------\n");
			return -1;
		}
		int tryToPfree = pfree(pmem);
		if (tryToPfree < 0)
		{
			printf(1, "--------------testingPmallocAndPfree FAILD: pfree failed--------------\n");
			return -1;
		}
		pmem = pmalloc();
		printf(1, "testingPmallocAndPfree: checking if possible to change unprotected pmalloc\n");
		int *testingChange = pmem;
		*testingChange = 123;
		printf(1, "testingPmallocAndPfree: checking if possible to acsses unprptected pmalloc\n");
		int i = *testingChange;
		protect_page(pmem);
		if (fork() == 0)
		{
			printf(1, "testingPmallocAndPfree: checking if  if in fork protected pages stay protected\n");
			printf(1, "testingPmallocAndPfree: trying to change protected memory in fork, expacting trap!!\n");
			*testingChange = 111;
		}
		else
		{
			sleep(10);
			wait();
			printf(1, "testingPmallocAndPfree: trying to change protected memory, expacting trap!!\n");
			*testingChange = i;
			*testingChange = 111;
		}
	}
	else
	{
		sleep(10);
		wait();
		printf(1, "--------------in case of trap: start pmalloc and pfree test PASSED--------------\n");
	}
	return 0;
}

int file_test()
{
	printf(1, "---------------- begin file_test test ----------\n");
	if (fork() == 0)
	{

		void *arr[25];
		for (int i = 0; i < 25; i++)
		{
			arr[i] = pmalloc();
		}
		for (int i = 0; i < 25; i++)
		{
			*(int *)arr[i] = i;
		}
		for (int i = 0; i < 25; i++)
		{
			printf(1, "value: %d ", *(int *)arr[i]);
		}
		for (int i = 0; i < 25; i++)
		{
			free(arr[i]);
		}
	}
	else
	{
		sleep(10);
		wait();
	}
	return 0;
	printf(1, "------------ success file_test test---------\n");
}

int testFork()
{
	printf(1, "--------------start fork test--------------\n");

	int total = PGSIZE * 5;
	int *mem = malloc(sizeof(int) * total);
	mem[0] = 1;
	mem[total - 1] = 2;

	printf(1, "data is: %d, %d\n", mem[0], mem[total - 1]);
	printf(1, "addresses are: %x , %x , %x \n", &mem[0], &mem[total - 1]);

	if (fork() == 0)
	{
		if (mem[0] != 1 || mem[total - 1] != 2)
		{
			printf(1, "\n---------fork FAILD, didnt copy memory------------ \n");
		}
		mem[0] = 5;
		mem[total - 1] = 6;

		printf(1, "data is: %d, %d\n", mem[0], mem[total - 1]);
		printf(1, "addresses are: %x , %x , %x \n", &mem[0], &mem[total - 1]);

		free(mem);
		exit();
	}
	else
	{
		wait();
		if (mem[0] != 1 || mem[total - 1] != 2)
		{
			printf(1, "\n---------fork FAILD, changed mommory in father------------  \n");
			free(mem);
		}
		else
		{
			printf(1, "\n---------fork test PASSD------------  \n");
			free(mem);
		}
		return 1;
	}
}
