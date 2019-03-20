#pragma once
#include "param.h"
#include "types.h"
#include "mmu.h"

// Per-CPU state
struct cpu
{
	uchar apicid;			   // Local APIC ID
	struct context *scheduler; // swtch() here to enter scheduler
	struct taskstate ts;	   // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS]; // x86 global descriptor table
	volatile uint started;	 // Has the CPU started?
	int ncli,				   // Depth of pushcli nesting.
		intena;				   // Were interrupts enabled before pushcli?
	struct proc *proc;		   // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context
{
	uint edi,
		esi,
		ebx,
		ebp,
		eip;
};

// Per-process state
struct proc
{
	uint sz;	  // Size of process memory (bytes)
	pde_t *pgdir; // Page table
	volatile enum procstate {
		UNUSED,
		EMBRYO,
		SLEEPING,
		RUNNABLE,
		RUNNING,
		ZOMBIE
	} state;							// Process state
	struct proc *parent;				// Parent process
	struct trapframe *tf;				// Trap frame for current syscall
	struct context *context;			// swtch() here to run process
	void *chan;							// If non-zero, sleeping on chan
	struct file *ofile[NOFILE];			// Open files
	struct inode *cwd;					// Current directory
	char *kstack,						// Bottom of kernel stack for this process
		name[16];						// Process name (debugging)
	int pid,							// Process ID
		killed,							// If non-zero, have been killed
		status,							// (added) Exit status
		priority;						// (added) priority between 1 to 10
	long long accumulator;				// (added) priority accumulator
	unsigned long long lastTickRunning, // (added) last tick proc was in the RUNNING state (by tiks1)
		firstTickRunnable,				// (added) first tick proc was in the RUNNABLE state (by ticks)
		firstTickRunning_by_ticks,		// (added) first tick proc was in the RUNNING state (by ticks)
		firstTickSleepping_by_ticks;	// (added) first tick proc was in the SLEEPPING state (by ticks)
	struct perf
	{
		unsigned long long ctime, // process creation time (technically should be int)
			ttime,				  // process termination time
			stime,				  // the total time the process spent in the SLEEPING state
			retime,				  // the total time the process spent in the RUNNABLE state
			rutime;				  // the total time the process spent in the RUNNING state
	} performance;				  // (added)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
