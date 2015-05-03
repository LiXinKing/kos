#include <types.h>
#include <s5pc110.h>
#include <stdio.h>
#include <uart.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>




extern int copy_mem(int ,struct task_struct * );
extern int copy_process(int );
extern int find_empty_process(void);
extern void ttest(unsigned long);
static inline _syscall0(int,fork)
static inline _syscall0(int,pause)
static inline _syscall0(int,getuid)

extern long kernel_mktime(struct tm *tm);

static void time_init(void)
{
	struct tm time;
	// 初始值为重置值
	unsigned long year = 12;
	unsigned long month = 5;
	unsigned long date = 1;
	unsigned long hour = 12;
	unsigned long min = 0;
	unsigned long sec = 0;

	unsigned long uread;

	//将时间转化为BCD码
	year = ( ((year/100)<<8) +(((year/10)%10)<<4) + (year%10)  );
	month  = ( ((month/10)<<4)+ (month%10) );
	date = ( ((date/10)<<4) + (date%10) );
	hour =( ((hour/10)<<4) + (hour%10) );
	min  =( ((min/10)<<4)  + (min%10) );
	sec  =( ((sec/10)<<4)  + (sec%10) );

	uread = RTCCON;
	RTCCON = (uread&~(1<<0))|(1);
	
	BCDSEC  = sec;
	BCDMIN  = min;
	BCDHOUR = hour;
	BCDDATE = date;
	BCDMON  = month;
	BCDYEAR = year;

	RTCCON = ( (uread&~(1<<8)) | (1<<8) );

	do {
		time.tm_sec = BCDSEC;
		time.tm_min = BCDMIN;
		time.tm_hour = BCDHOUR;
		time.tm_mday = BCDDATE;
		time.tm_mon = BCDMON;
		time.tm_year =BCDYEAR;

		}while(time.tm_sec != BCDSEC);
		
		time.tm_mon--;
		startup_time = kernel_mktime(&time);
	printf("startup_time is    %ld\n",startup_time);
}

static void delay(void)
{	
	int i=0,j=0,k=0;
	for(;i<1000000;i++)
		for(;j<1000000;j++)
			k++;
	return;
}
void enable_interrupts(void)
{
	unsigned long temp;
	__asm__ __volatile__("mrs %0, cpsr\n" "bic %0, %0,#0x80\n" "msr cpsr_c, %0":"=r"(temp)
						::"memory");
	return;
}
void put_to()
{
	char a='k';
	putc(a);
	putc('b');
	putc('c');
	putc('a');
	return;
}
void start_armboot (void)
{	
//	GPBCON_REG=0x00001111;
//	GPBDAT_REG=0xff;
//	while(1);
	uart_init();
	printf("IN?\n");
	
	system_initexception();
	sched_init();
	mem_init(0x20a00000,0x24000000);
	time_init();
//	__asm__ __volatile__("msr cpsr_c,#(0x80|0x40|0x10)":::);
	//到这里以后这个函数是不可能会返回的，在这里把堆栈换掉，应该不会出现问a
/*	unsigned long sp0=4096+(unsigned long)&init_task;
	unsigned long sp1=(unsigned long)stack_start;
	printf("sp0    %X\n",sp0);
	printf("sp1    %X\n",sp1);*/
//	enable_interrupts();
//这个cpu是小端的

	move_to_usr(4096+(unsigned long)&init_task,4096+(unsigned long)stack_start,4096+(long)stack_ex);
	printf("OK IN start_armboot\n");
//	move_to_usr(1);
//	printf("tm\n");	
	unsigned long tmp=1;
	unsigned long tt=1;
	asm("mov %0,sp":"=r" (tmp));
	asm("mrs %0,CPSR":"=r" (tt));
	printf("%X\n",tmp);
	printf("tt             %X\n",tt);
	printf("fork ok ?\n");


	if(!fork())
	{
		init();
	}


	printf("father process\n");
//任务0为未就绪状态然后执行调度
	while(1)
	{
		pause();
		}
 }
 //任务1开始的地方
 void init(void)
{	
	if(!fork())
	{
		init2();

	}
	char j='k';
	while(1)
		{
			if(j=='k')j='m';
			else j='k';
		//	putc(j);
		}
}
void init2(void)
{
			init3();

}

void init3(void)
{
	int uuid=1;
	uuid=getuid();
	printf("get out uid : %d\n",uuid);
	while(1);
}






