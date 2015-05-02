/*************************************************************************
	> File Name: sched.h
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年06月23日 星期一 14时37分03秒
 ************************************************************************/
/*#include "signal.h"*/
#include <sys/resource.h>

#define HZ 100
#define NR_TASKS	64
#define TASK_SIZE	0x04000000

#define ParSaved 52
extern struct task_struct *current;
extern union task_union init_task;
extern struct task_struct * task[NR_TASKS];
extern char stack_start[4096];
extern char stack_ex[4096];

extern unsigned long volatile jiffies;
extern unsigned long startup_time;
extern int jiffies_offset;



extern void panic(const char * str);
#define CT_TO_SECS(x)	((x) / HZ)
#define CT_TO_USECS(x)	(((x) % HZ) * 1000000/HZ)

struct cpu_contex_save {
	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;
	unsigned long r8;
	unsigned long r9;
	unsigned long sl; //r10
	unsigned long fp; //r11
	unsigned long sp; //r13
	unsigned long lr; //r14
	unsigned long pc; //r15
	unsigned long sp_usr;
};
struct task_struct {
/* these are hardcoded - don't touch */
	unsigned long sp0;//for kernel
	unsigned long sp1;//for user
	unsigned long sp2;//for irq etc.
	int nr;
	int ntask;
	long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	long counter;
	long priority;
//	long signal;
//	struct sigaction sigaction[32];
///	long blocked;	/* bitmap of masked signals */
/* various fields */
//	int exit_code;
	unsigned long start_code,end_code,end_data,brk,start_stack;
	long pid,pgrp,session,leader;
//	int	groups[NGROUPS];
	/* 
	 * pointers to parent process, youngest child, younger sibling,
	 * older sibling, respectively.  (p->father can be replaced with 
	 * p->p_pptr->pid)
	 */
	struct task_struct	*p_pptr, *p_cptr, *p_ysptr, *p_osptr;
	unsigned short uid,euid,suid;
	unsigned short gid,egid,sgid;
//	unsigned long timeout,alarm;
	long utime,stime,cutime,cstime,start_time;
	struct rlimit rlim[RLIM_NLIMITS]; 
//	unsigned int flags;	/* per process flags, defined below */
/* file system info */
	int tty;		/* -1 if no tty, so it must be signed */
	unsigned short umask;
//	struct m_inode * pwd;
//	struct m_inode * root;
//	struct m_inode * executable;
//	struct m_inode * library;
//	unsigned long close_on_exec;
//	struct file * filp[NR_OPEN
/* tss for this task */
//	struct tss_struct tss;
	unsigned long pgt;
//	struct cpu_contex_save cpu_contex_save;
};
#define INIT_TASK \
{4096+(long)&init_task,(long)stack_start,(long)stack_ex,\
/*nr,ntask*/	0,0, \
/*state*/	0,1,1, \
/*start_code*/	0,0,0,0,0, \
/*pid~lead*/	0,0,0,0,\
/*父指针等*/	&init_task.task,0,0,0, \
/*uid~sgid*/	0,0,0,0,0,0,\
/*utime~start_time*/	0,0,0,0,0,0, \
/* rlimits */   { {0x7fffffff, 0x7fffffff}, {0x7fffffff, 0x7fffffff},  \
		  {0x7fffffff, 0x7fffffff}, {0x7fffffff, 0x7fffffff}, \
				  {0x7fffffff, 0x7fffffff}, {0x7fffffff, 0x7fffffff}}, \
/*tty!!*/	-1,	0002,\
/*pgt~~*/	0x20000000 \
}


#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4

#ifndef NULL
#define NULL ((void *) 0)
#endif
#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]
//#define INIT_SP0 4096+(unsigned long)&init_task
//#define INIT_SP1 (unsigned long)stack_start
//	{0,0,0,0,0,0,0,fp,(char *)task[0]+4096,lr,pc,stack_start},
/*#define move_to_usr(n) \
	unsigned long tmp1=4096+(unsigned long)&init_task; \
	unsigned long tmp2=(unsigned long)stack_start;      \
	__asm__( \
			"mov sp,%0\n" \
			"mrs r0,CPSR\n" \
			"bic r0,r0,#0xF\n" \
			"msr CPSR,r0\n" \
			"movsp,%1\n" \
			: \
			:"r" (tmp1), "r" (tmp2)\
			:"r0"\
	) 
*/
