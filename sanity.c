// #include "types.h"
#include "user.h"

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
}