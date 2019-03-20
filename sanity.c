// #include "types.h"
#include "user.h"
#include"proc.h"

int main(void)
{
	// 	int pid = fork();
	// 	if (pid > 0)
	// 	{
	// 		if (pid == 9)
	// 		{
	// 			printf(1, "detach :%d -> should be 0\n", detach(pid));
	// 			printf(1, "detach :%d -> should be -1\n", detach(3));
	// 		}
	// 		else
	// 		{
	// 			printf(1, "detach :%d -> should be 0\n", detach(pid));
	// 			printf(1, "detach :%d -> should be -1\n", detach(9));
	// 		}
	// 	}
	/*

int pid = fork();
if (pid == 0) {
sleep(300);
exit(5);
}
else {
int* status = malloc(1);
wait(status);
printf(1, "exit status after is %d\n",*status);
}

exit(0);
*/
	// struct perf
	// {
	// 	unsigned long long ctime, // process creation time (technically should be int)
	// 		ttime,				  // process termination time
	// 		stime,				  // the total time the process spent in the SLEEPING state
	// 		retime,				  // the total time the process spent in the RUNNABLE state
	// 		rutime;				  // the total time the process spent in the RUNNING state
	// };

	struct perf perf1;

	int pid = fork();
	if (pid == 0)
	{
		printf(1, "Child is waiting\n");
		sleep(300);
		exit(5);
	}
	else
	{
		int status;
		wait_stat(&status, &perf1);
		printf(1, "exit status after is %d\n", status);
		printf(1, "ctime is %d\n", perf1.ctime);
		printf(1, "ttime is %d\n", perf1.ttime);
		printf(1, "stime is %d\n", perf1.stime);
		printf(1, "retime is %d\n", perf1.retime);
		printf(1, "rutime is %d\n", perf1.rutime);
	}

	exit(0);

	/*

	int pid = fork();
	if (pid > 0)
	{
		int first_status = detach(pid),  // status = 0
			second_status = detach(pid), // status = -1, because this process has already
										 // detached this child, and it doesn’t have
										 // this child anymore.
			third_status = detach(77); // status = -1, because this process doesn’t
									   // have a child with this pid.
		printf(1, "first_status = %d\n"
				  "second_status = %d\n"
				  "third_status = %d\n",
			   first_status, second_status, third_status);
	}
	exit(0);
	*/
}