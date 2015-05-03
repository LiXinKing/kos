/************************************************************************
	> File Name: timer.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年06月17日 星期二 15时40分00秒
 ************************************************************************/
#include <types.h>
#include <s5pc110.h>
#include <stdio.h>
#include <interrupt.h>
#include <sched.h>
// 用于记录中断发生的次数
int counter=0;
extern void schedule(void);
// 停止所有timer,往con控制器中写0的话可以关闭所有的
void pwm_stopall(void)
{
	TCON_REG = 0;
	}
void irs_timer(unsigned long pl)
{
	unsigned long uTmp;
	jiffies++;
	//清timer0的中断状态寄存器
	uTmp = TINT_CSTAT;
	TINT_CSTAT = uTmp;      
	// 打印中断发生次数
	printf("pl:     %X\n",pl);
	printf("Timer0IntCounter = %d \n",counter++);
	uTmp=pl&(0xF);


	if (uTmp)
		current->stime++;
	else
		current->utime++;
	
	// vic相关的中断清除
	intc_clearvectaddr();
	if ((--current->counter)>0) return;
		current->counter=0;
	if (uTmp) return;	//由内核态进入时间中断，不执行切换
	schedule();						
		}


void timer_init(unsigned long utimer,unsigned long uprescaler,unsigned long udivider,unsigned long utcntb,unsigned long utcmpb)
{
	unsigned long temp0;

	// 定时器的输入时钟 = PCLK / ( {prescaler value + 1} ) / {divider value} = PCLK/(65+1)/16=62500hz
	//时钟系数的主要设置在这里
	//设置预分频系数为66
	temp0=TCFG0_REG;
	temp0 = (temp0 & (~(0xff00ff))) | ((uprescaler-1)<<0);
	TCFG0_REG=temp0;

	// 16分频，分频是通过这里的16分频和上面的预分频，并通过公式
	// 定时器的输入时钟 = PCLK / ( {prescaler value + 1} ) / {divider value} 
	//= PCLK/(65+1)/16=62500hz,来实现的

	temp0=TCFG1_REG;
	temp0 = (temp0 & (~(0xf<<4*utimer))& (~(1<<20))) |(udivider<<4*utimer);
	TCFG1_REG=temp0;

		// 1s = 62500hz，没1/62500s就将TCNTB0-1，直到它和TCMPB0相等就产生中断
	TCNTB0_REG = utcntb;
	TCMPB0_REG = utcmpb;

		// 手动更新,这里必须先手动更新一下然后清手动更新位，设置自动更新位，然后能正常的自动装载
	TCON_REG |= 1<<1;

		// 清手动更新位
	TCON_REG &= ~(1<<1);

		// 自动加载和启动timer0
	TCON_REG |= (1<<0)|(1<<3);

		// 使能timer0中断，这里的使能中断和上面的不一样的，上面是使能大的终端如vc0，vc1等
	temp0 = TINT_CSTAT;
	temp0 = (temp0 & (~(1<<utimer)))|(1<<(utimer));
	TINT_CSTAT = temp0;
}
void timer_request(void)
{
	printf("\n######Timer test####\n");
//	printf("start\n");
//	printf("s3ff\n");
	// 禁止所有timer
	pwm_stopall();								

	counter = 0;

	// 设置timer0中断的中断处理函数，设置最终的终端处理函数（最后一步）
	intc_setvectaddr(NUM_TIMER0,irs_timer);	
							
	// 使能timer0中断
	intc_enable(NUM_TIMER0);
										
	// 设置timer0
	timer_init(0,65,4,6250*2,0);		
}
