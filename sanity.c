#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
	int pid = fork();
	if (pid > 0)
	{
		if (pid == 9)
		{
			printf(1, "detach :%d -> should be 0\n", detach(pid));
			printf(1, "detach :%d -> should be -1\n", detach(3));
		}
		else
		{
			printf(1, "detach :%d -> should be 0\n", detach(pid));
			printf(1, "detach :%d -> should be -1\n", detach(9));
		}
	}
	exit(0);
}