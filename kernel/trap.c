/*************************************************************************
	> File Name: trap.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年06月19日 星期四 10时49分33秒
 ************************************************************************/

#include <stdio.h>
#include <interrupt.h>
//异常将被设置成不可抢占
void do_page_fault(char *,int,unsigned long);
void do_no_table(char *,int,unsigned long);
void do_no_page(char *,int,unsigned long);
static void do_bad(char *,int,unsigned long);
unsigned long intc_getvicirqstatus(unsigned long );
static struct dataabort_fun{
	void (*fn)(char *name,int domain,unsigned long addr);
	char *name;
	int select;
}dataabort[]={
	{do_bad,"data:alignment fault\n",1},
	{do_bad,"data:instruction cache maintenance fault\n",4},
	{do_bad,"data:L1 translation ,precise external abort\n",12},
	{do_bad,"data:L2 translation ,precise external abort\n",14},
	{do_bad,"data:L1 translation precise parity error\n",28},
	{do_bad,"data:L2 translation precise parity error\n",30},
	{do_no_table,"data:translation fault, section\n",5},
	{do_no_page,"data:translation fault, page\n",7},
	{do_bad,"data:access flag fault,section\n",3},
	{do_bad,"data:access flag fault, page\n",6},
	{do_bad,"data:domain fault, section\n",9},
	{do_bad,"data:domain fault, page\n",11},
	{do_bad,"data:permission fault, section\n",13},
	{do_page_fault,"data:permission fault, page\n",15},
	{do_bad,"data:precise external abort, nontranslation\n",8},
	{do_bad,"data:imprecise external abort\n",22},
	{do_bad,"data:imprecise error.parity or ECC\n",24},
	{do_bad,"data:debug event\n",2}
};


static struct instrcabort_fun{
	void (*fn)(char *name,int domain,unsigned long addr);
	char *name;
	int select;
}instrcabort[]={
	{do_bad,"instruction:L1 translation ,precise external abort\n",12},
	{do_bad,"instruction:L2 translation ,precise external abort\n",14},
	{do_bad,"instruction:L1 translation precise parity error\n",28},
	{do_bad,"instruction:L2 translation precise parity error\n",30},
	{do_bad,"instruction:translation fault, section\n",5},
	{do_no_page,"instruction:translation fault, page\n",7},
	{do_bad,"instruction:access flag fault,section\n",3},
	{do_bad,"instruction:access flag fault, page\n",6},
	{do_bad,"instruction:domain fault, section\n",9},
	{do_bad,"instruction:domain fault, page\n",11},
	{do_bad,"instruction:permission fault, section\n",13},
	{do_bad,"instruction:permission fault, page\n",15},
	{do_bad,"instruction:precise external abort, nontranslation\n",8},
	{do_bad,"instruction:precise parity error\n",25},
	{do_bad,"instruction:debug event\n",2}
};


static void do_bad(char *name,int domain,unsigned long addr)
{
	printf(name);
	printf("if data abort then caused in domain ");
	printf("%d",domain);
	printf(" Address: 0x");
	printf("%X\n",addr);
	while(1);
}

void exceptionundefins(void)
{
	printf("undefined instrcution error..abort\n");
	while(1);
}

void exceptionsoftwareinter(void)
{

	printf("software interrupt ...remained\n");
	while(1);
}

void exceptionprefetch(void)
{
	printf("prefetch error...abort\n");
	unsigned long ex_type;
	unsigned long addr;
	__asm__ __volatile__("mrc p15,0,r0,c5,c0,1\n" "mov %0,r0\n":"=r"(ex_type)
						::"memory");
	__asm__ __volatile__("mrc p15,0,r0,c6,c0,2\n" "mov %0,r0\n":"=r"(addr)
						::"memory");
//	printf("111");
	unsigned long select=ex_type&(0xF)+((ex_type&(0x400))?16:0);	
	printf("pre addr :   %X\n",addr);
	printf("pre select:   %X\n",select);
	int domain=17;
	void (*isr)(char *,int,unsigned long) = NULL;
	char *name=NULL;
	int i=0;
	while(i<15)
	{	
		i++;
		if(instrcabort[i].select!=select)continue;
		isr=instrcabort[i].fn;
		name=instrcabort[i].name;
		break;
	}
	isr(name,domain,addr);
}

void exceptiondataabort(void)
{

	
	printf("data error ....abort\n");
//	while(1);
	volatile unsigned long ex_type;
	volatile unsigned long addr=0;
	__asm__("mrc p15,0,r0,c5,c0,0\n" "mov %0,r0\n":"=r"(ex_type)
						::"memory");

	__asm__("mrc p15,0,r0,c6,c0,0\n" "mov %0,r0\n":"=r"(addr)
						::"memory");
	printf("type:    %X   \n",ex_type);
	printf("addr:    %X   \n",addr);
//	printf("1239999");
//	while(1);

	unsigned long select=ex_type&(0xF)+((ex_type&(0x400))?16:0);	
	int domain=ex_type&(0xF0)>>4;
	void (*isr)(char *,int,unsigned long) = NULL;
	char *name=NULL;
	int i=0;
	while(i<18)
	{	
		i++;
		if(dataabort[i].select!=select)continue;
		isr=dataabort[i].fn;
		name=dataabort[i].name;
		break;
	}
	isr(name,domain,addr);
}
void exceptionfiq(void)
{
	printf("fiq interrupt ....remained\n");
	while(1);

}
// 通用中断处理函数,这里是中断函数的第二个跳转点，即从start.S到这里
void exceptionirq(unsigned long pl)
{
	putc('m');
	printf("irq not defined.\n");
	unsigned long tmp=1;
	unsigned long tt=1;
	asm("mov %0,sp\n":"=r"(tmp));
	asm("mrs %0,CPSR\n":"=r"(tt));
	printf("%X\n",tmp);
	printf("tt             %X\n",tt);
	unsigned long vicaddr[4] = {VIC0ADDR,VIC1ADDR,VIC2ADDR,VIC3ADDR};
	int i=0;
	void (*isr)(unsigned long) = NULL;

	for(; i<4; i++)
	{
		if(intc_getvicirqstatus(i) != 0)
		{
			isr = (void (*)(unsigned long)) vicaddr[i];
			break;
		}
	}
	(*isr)(pl);
}
