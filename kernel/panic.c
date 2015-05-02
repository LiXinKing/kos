/*************************************************************************
	> File Name: panic.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年07月27日 星期日 20时42分20秒
 ************************************************************************/
#include <sched.h>

//void sys_sync(void);	/* it's really int */

volatile void panic(const char * s)
{
	printf("Kernel panic: %s\n\r",s);
//	if (current == task[0])
//		printk("In swapper task - not syncing\n\r");
//	else
//		sys_sync();
	for(;;);
}


