#pragma once
#include "types.h"
#include "sleeplock.h"
#include "fs.h"
struct buf
{
	int flags;
	uint dev;
	uint blockno;
	struct sleeplock lock;
	uint refcnt;
	struct buf *prev; // LRU cache list
	struct buf *next;
	struct buf *qnext; // disk queue
	uchar data[BSIZE];
};

enum BState
{
	B_VALID = 0x2,
	B_DIRTY = 0x4
};
// #define B_VALID 0x2 // buffer has been read from disk
// #define B_DIRTY 0x4 // buffer needs to be written to disk
