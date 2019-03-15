// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "isFileExists.h"

char *argv[] = {"sh", 0};

int main(void)
{
	int pid, wpid;

	if (open("console", O_RDWR) < 0)
	{
		mknod("console", 1, 1);
		open("console", O_RDWR);
	}
	dup(0); // stdout
	dup(0); // stderr

	if (!isFileExists("path"))
	{
		int fd = open("path", O_CREATE | O_WRONLY);
		write(fd, "/:/bin/:\0", 9);
		close(fd);
	}

	for (;;)
	{
		printf(1, "init: starting sh\n");
		pid = fork();
		if (pid < 0)
		{
			printf(1, "init: fork failed\n");
			exit(0);
		}
		if (pid == 0)
		{
			exec("sh", argv);
			printf(1, "init: exec sh failed\n");
			exit(0);
		}
		while ((wpid = wait(null)) >= 0 && wpid != pid)
			printf(1, "zombie!\n");
	}
}
