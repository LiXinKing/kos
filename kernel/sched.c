/*************************************************************************
	> File Name: sched.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年06月23日 星期一 11时26分39秒
 ************************************************************************/
#include <sched.h>


union task_union{
	struct task_struct task;
	char stack[4096];
};

union task_union init_task = {INIT_TASK,};
struct task_struct * task[NR_TASKS] = {&(init_task.task), };
struct task_struct *current = &(init_task.task);
char stack_start[4096];
char stack_ex[4096];


unsigned long volatile jiffies=0;
unsigned long startup_time=0;
int jiffies_offset=0;

void sched_init(void)
{

	timer_request();

}
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;
	
	printf("In schedule \n");
	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
			{	
				c = (*p)->counter;
				next = i;
			}
		}
		if (c) break;
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	printf("task:text: %x\n",next);
	switch_to(current,task[next]);

}
static inline void __sleep_on(struct task_struct **p, int state)
{
	struct task_struct *tmp;

	if (!p)
	return;
	if (current == &(init_task.task))
	panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = state;
repeat:	schedule();
	if (*p && *p != current) {
		(**p).state = 0;
		current->state = TASK_UNINTERRUPTIBLE;
		goto repeat;
				}
	if (!*p)
	printf("Warning: *P = NULL\n\r");
	if (*p = tmp)
	tmp->state=0;
}

void interruptible_sleep_on(struct task_struct **p)
{
	__sleep_on(p,TASK_INTERRUPTIBLE);
}


void sleep_on(struct task_struct **p)
{
	__sleep_on(p,TASK_UNINTERRUPTIBLE);
}


void wake_up(struct task_struct **p)
{
	if (p && *p) {
		if ((**p).state == TASK_STOPPED)
			printf("wake_up: TASK_STOPPED");
		if ((**p).state == TASK_ZOMBIE)
			printf("wake_up: TASK_ZOMBIE");
			(**p).state=0;
					}
}


void ttest(unsigned long pp)
{	
	unsigned long pp0;
	int i=0;
	printf("ttest:     %X\n",pp);
	while(i<0){
	pp0=*(volatile unsigned long *)(pp+i*4);
	printf("this data :    %X\n",pp0);
	i++;
	}
	return;
}
int sys_pause(void)
{
	printf(" in pause \n");
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

int sys_getpid(void)
{
	return current->pid;
}

int sys_getppid(void)
{
	return current->p_pptr->pid;
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

