/*************************************************************************
	> File Name: fork.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年07月01日 星期二 20时29分51秒
 ************************************************************************/
#include <sched.h>
#include <mm.h>
#include <string.h>
//#include "string.h"

long last_pid=0;
extern void write_verify(unsigned long);

unsigned long  get_taskmm(void)
{
	unsigned long tmp;
	tmp=current->nr;
	tmp=tmp*0x4000000;
	return tmp;
}


void verify_area(void * addr,int size)
{
	unsigned long start;

	start = (unsigned long) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_taskmm();
//	start += get_base(current->ldt[2]);
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}


int copy_mem(int nr,struct task_struct * p)
{	

	unsigned long old_data_base,new_data_base,data_limit;
//	unsigned long *addr;
//	addr=((unsigned long *)(pg_dir_base+nr*16*1024));
//	printf("pgt:%X\n",current->pgt);
//	printf("pgt_addr:%X\n",addr);
//	memcpy(addr,current->pgt,16*1024);
/*	int j=0;
	int k=0;
	unsigned long *tt1=(unsigned long *)(addr);
	unsigned long *tt2=(unsigned long *)(current->pgt);
	while(j<4096)
	{
		if(*tt1==*tt2)k++;
		j++;
		tt1++;
		tt2++;
	
	}
	printf("k:    %X\n",k);*/

	old_data_base=current->nr*TASK_SIZE;
	new_data_base=nr*TASK_SIZE;
	printf("old_data_base:    %X\n",old_data_base);
	printf("new_data_base:    %X\n",new_data_base);

	if(copy_page_tables(old_data_base,new_data_base,Mem_Limited))
	{
		free_page_tables(new_data_base,Mem_Limited);
		return -1;
		
	}
	return 0;
}



int copy_process(int nr)
{
	printf("OK\n");
	struct task_struct *p;
	p = (struct task_struct *) get_free_page();
	printf("OK1\n");
	if(!p)
		return -1;
	p=(struct task_struct *)phy_to_vir(p);
	printf("P:    %X\n",p);
	printf("nr:      %X\n",nr);
	task[nr]=p;
	printf("OK2\n");
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	p->state = TASK_UNINTERRUPTIBLE;
	
	p->pid = last_pid;
	p->counter = p->priority;
	p->sp0=(long)p+PAGE_SIZE-ParSaved;
	p->nr=nr;
	p->ntask=1;
	p->start_time = jiffies;
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;


	p->p_pptr = current;
	p->p_cptr = 0;
	p->p_ysptr = 0;
	p->p_osptr = current->p_cptr;
	if (p->p_osptr)
		p->p_osptr->p_ysptr = p;
	current->p_cptr = p;
	p->state = TASK_RUNNING;	/* do this last, just in case */
	
	if (copy_mem(nr,p)) {
		task[nr] = 0;
		free_page((long) p);
		return -1;
				}
	
	return p->sp0+ParSaved;


}
int find_empty_process(void)
{
	int i;

	repeat:
	if ((++last_pid)<0) last_pid=1;
	for(i=0 ; i<NR_TASKS ; i++)
	if (task[i] && ((task[i]->pid == last_pid) ||
					(task[i]->pgrp == last_pid)))
	goto repeat;
	
	for(i=1 ; i<NR_TASKS ; i++)
	if (!task[i])
	return i;
	return -1;
}

//void sys_fork()
//{
//	after_test();
//	return;
//}
