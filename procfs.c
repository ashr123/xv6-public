#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

void itoa(char *s, int n)
{//make int to string
	int i, len;
	i = len = 0;
	if (n == 0)
	{
		s[0] = '0';
		s[1] = '\0';
		return;
	}
	while (n != 0)
	{
		s[len] = n % 10 + '0';
		n = n / 10;
		len++;
	}
	for (i = 0; i < len / 2; i++)
	{
		char tmp = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = tmp;
	}
	s[len] = 0;
}

void writeNumber(char *to, int number)
{
	int size, add;
	char buff[7];
	itoa(buff, number);
	size = strlen(buff);
	add = strlen(to);
	memmove(to + add, buff, size);
	*(to + add + size) = '\0';
}

void writeText(char *to, char *text)
{
	int size, add;
	size = strlen(text);
	add = strlen(to);
	memmove(to + add, text, size);
	*(to + add + size) = '\0';
}

void writeDirectoryEntry(char *to, int inum, char *name, int length)
{
	struct dirent dir;
	int end = 1;
	dir.inum = inum;
	memmove(dir.name, name, strlen(name) + end);
	memmove(to + length * sizeof(dir), (void *) &dir, sizeof(dir));
}


int read_file_by_case(struct inode *ip, char *dest, int case_)
{
	if (case_ == 1)
	{ //idinfo = info about read write etc..
		int fs[3];
		char fes[2000];
		get_idinfo_in_num(fs);
		get_idinfo_in_text(fes);
		*dest = '\0';
		writeText(dest, "Waiting operations: ");
		writeNumber(dest, fs[2]);
		writeText(dest, "\n");
		writeText(dest, "Read waiting operations: ");
		writeNumber(dest, fs[1]);
		writeText(dest, "\n");
		writeText(dest, "Write waiting operations: ");
		writeNumber(dest, fs[0]);
		writeText(dest, "\n");
		writeText(dest, "Working blocks: ");
		char inf[2000];
		get_idinfo_in_text(inf);
		writeText(dest, inf);
		return strlen(dest);
	} else if (case_ == 2)
	{//file state
		int result[5];
		*dest = '\0';
		get_open_file_info(result);
		writeText(dest, "Free fds: ");
		writeNumber(dest, result[0]);
		writeText(dest, "\n");
		writeText(dest, "Unique inode fds: ");
		writeNumber(dest, result[1]);
		writeText(dest, "\n");
		writeText(dest, "Writable fds: ");
		writeNumber(dest, result[2]);
		writeText(dest, "\n");
		writeText(dest, "Readable fds: ");
		writeNumber(dest, result[3]);
		writeText(dest, "\n");
		writeText(dest, "Refs per fds: ");
		writeNumber(dest, result[4]);
		return strlen(dest);
	} else if (case_ == 3)
	{ //inod info
		struct inode *node = get_inode_by_index(ip->inum - 30000);
		*dest = '\0';
		writeText(dest, "Device: ");
		writeNumber(dest, node->dev);
		writeText(dest, "\nInode number: ");
		writeNumber(dest, node->inum);
		writeText(dest, "\nis valid: ");
		writeNumber(dest, node->valid);
		writeText(dest, "\ntype: ");
		if (node->type == 1)
		{
			writeText(dest, "DIR");
		} else if (node->type == 2)
		{
			writeText(dest, "FILE");
		} else
		{
			writeText(dest, "DEV");
		}

		writeText(dest, "\nmajor minor: ");
		writeNumber(dest, node->major);
		writeText(dest, ", ");
		writeNumber(dest, node->minor);
		writeText(dest, "\nhard links: ");
		writeNumber(dest, node->nlink);
		writeText(dest, "\nblocks used: ");
		if (node->type == 3)
		{
			writeNumber(dest, 0);
		} else if (node->size % 512 == 0)
		{
			writeNumber(dest, node->size / 512);
		} else
		{
			writeNumber(dest, (node->size / 512) + 1);
		}
		writeText(dest, "\n");
		return strlen(dest);
	} else if (case_ == 4)
	{ //proc name
		int pid = (ip->inum - 1) / 100;
		struct proc *p = get_proc(pid);
		memmove(dest, p->name, strlen(p->name));
		return strlen(p->name);
	} else if (case_ == 5)
	{//proc status
		int pid = (ip->inum - 1) / 100;
		struct proc *p = get_proc(pid);
		char *states[] = {"UNUSED    ", "EMBRYO    ", "SLEEPING  ", "RUNNABLE  ", "RUNNING   ", "ZOMBIE    "};
		memmove(dest, states[p->state], strlen(states[p->state]));
		char n[12];
		itoa(n, p->sz);
		memmove(dest + 10, n, strlen(n));
		return 10 + strlen(n);
	} else
		panic("read file failed!!!");


}

int read_file(struct inode *ip, char *dst)
{

	int case_ = 0; //no case
	if (ip->inum == 97) //idinfo = info about read write etc..
		case_ = 1;
	else if (ip->inum == 98) //file state
		case_ = 2;
	else if (ip->inum >= 30000) //inod info
		case_ = 3;
	else if (ip->inum % 100 == 1) //proc name
		case_ = 4;
	else if (ip->inum % 100 == 2) //proc status
		case_ = 5;
	return read_file_by_case(ip, dst, case_);

}


///////////////////////////////////////////////
int read_dir(struct inode *ip, char *dest)
{ //print dir content
	int case_ = 0;//no case

	if (ip->inum == namei("/proc")->inum) //main proc dir
		case_ = 1;
	else if (ip->inum == 99) //inodinfo dir
		case_ = 2;
	else if (ip->inum % 100 == 0) //proccess dir
		case_ = 3;

	int processes[NPROC];
	int length_running = get_all_running_proceess(processes);
	int inodes[NINODE];
	int length_inodes = get_indexes_of_inodes(inodes);
	int i;
	switch (case_)
	{
		case 1:
			writeDirectoryEntry(dest, ip->inum, ".", 0);
			writeDirectoryEntry(dest, namei("/")->inum, "..", 1);
			writeDirectoryEntry(dest, 97, "ideinfo", 2);
			writeDirectoryEntry(dest, 98, "filestat", 3);
			writeDirectoryEntry(dest, 99, "inodeinfo", 4);
			for (i = 0; i < length_running; i++)
			{
				char n[6];
				itoa(n, processes[i]);
				writeDirectoryEntry(dest, processes[i] * 100, n, 5 + i);
			}
			return (5 + length_running) * sizeof(struct dirent);

		case 2:
			writeDirectoryEntry(dest, ip->inum, ".", 0);
			writeDirectoryEntry(dest, namei("/proc")->inum, "..", 1);
			for (i = 0; i < length_inodes; i++)
			{
				char n[6];
				itoa(n, inodes[i]);
				writeDirectoryEntry(dest, 30000 + inodes[i], n, 2 + i);
			}
			return (2 + length_inodes) * sizeof(struct dirent);

		case 3:
			writeDirectoryEntry(dest, ip->inum, ".", 0);
			writeDirectoryEntry(dest, namei("/proc")->inum, "..", 1);
			writeDirectoryEntry(dest, ip->inum + 1, "name", 2);
			writeDirectoryEntry(dest, ip->inum + 2, "status", 3);
			return 4 * sizeof(struct dirent);
		default:
			panic("no case read directory!");
	}

}

//old:
int
procfsisdir(struct inode *ip)
{ //check if inode is file or dir
	return ip->inum == namei("/proc")->inum || ip->inum == 99 || (ip->inum % 100 == 1 || ip->inum % 100 == 2);
}

void
procfsiread(struct inode *dp, struct inode *ip)
{ //initial inode`s fileds
	ip->type = T_DEV;
	ip->major = PROCFS;
	ip->nlink = 1;
	ip->valid = 1;

}


int
procfsread(struct inode *ip, char *dst, int off, int n)
{ //read from files
	char buf[2048];
	int ret;
	//  if (ip->inum == namei("/proc")->inum || ip->inum == 99 || (ip->inum % 100 == 0 && ip->inum != 30000))
	int is_dir = ((ip->inum == namei("/proc")->inum) || (ip->inum == 99) || (ip->inum % 100 == 0 && ip->inum != 30000));
	if (is_dir)
		ret = read_dir(ip, buf);
	else
		ret = read_file(ip, buf);
	memmove(dst, buf + off, n);
	if (n < (ret - off))
	{
		return n;
	} else
	{
		return ret - off;
	}
}

int
procfswrite(struct inode *ip, char *buf, int n) //sopopse to write to files
{
	return 0;
}

void
procfsinit(void)
{
	devsw[PROCFS].isdir = procfsisdir;
	devsw[PROCFS].iread = procfsiread;
	devsw[PROCFS].write = procfswrite;
	devsw[PROCFS].read = procfsread;
}
