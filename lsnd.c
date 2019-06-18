
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"


void
test(void)
{

	struct dirent dir[72];
	char name_buffer[30];
	char content_buffer[300];
	int i, j, count, readN, inode_filed, fd;
	count = i = readN = 0;
	fd = open("/proc/inodeinfo", O_RDONLY);
	read(fd, dir, 2 * sizeof(struct dirent));
	while (read(fd, &dir[count++], sizeof(struct dirent)));
	memmove(name_buffer, "/proc/inodeinfo/", 16);
	for (i = 0; i < count - 1; i++)
	{
		readN = 0;
		memmove(name_buffer + 16, dir[i].name, strlen(dir[i].name));
		*(name_buffer + 16 + strlen(dir[i].name)) = 0;
		inode_filed = open(name_buffer, O_RDONLY);
		read(inode_filed, content_buffer, 300);
		for (j = 0; j < 7; j++)
		{
			for (; content_buffer[readN] != ':'; readN++);
			readN = readN + 2;
			for (; content_buffer[readN] != '\n'; readN++)
			{
				write(1, content_buffer + readN, 1);
			}
			readN = readN + 1;
			printf(1, " ");
		}
		printf(1, "\n");
	}
}

int
main(void)
{
	test();
	exit();
}
