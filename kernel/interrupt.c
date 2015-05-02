/*************************************************************************
	> File Name: interrupt.c
	> Author: jinlx
	> Mail: a1307468@126.com 
	> Created Time: 2014年06月17日 星期二 14时37分58秒
 ************************************************************************/
#include <interrupt.h>
#include <stdio.h>

// 中断相关初始化
void system_initexception( void)
{
	// 设置中断向量表，这个中断触发后跳转的第一个地方
	//然后由这个地方跳到指定的中断函数进行处理，这个函数在start.S中
//	undefined_instruction();
	
//	pExceptionUNDEF	  =	(unsigned long )exceptionundef;
//	pExceptionSWI  	  =	(unsigned long )exceptionswi;
//	pExceptionPABORT  =	(unsigned long )exceptionpabort;
//	pExceptionDABORT  =	(unsigned long )exceptiondabort;
//	pExceptionIRQ	  =	(unsigned long )irq;
//	pExceptionFIQ	  =	(unsigned long )exceptionfiq;

	// 初始化中断控制器
	intc_init();

}



// 初始化中断控制器  
void intc_init(void)
{
	// 禁止所有中断
	VIC0INTENCLEAR = 0xffffffff;
	VIC1INTENCLEAR = 0xffffffff;
	VIC2INTENCLEAR = 0xffffffff;
	VIC3INTENCLEAR = 0xffffffff;

	// 选择中断类型为IRQ
	VIC0INTSELECT = 0x0;
	VIC1INTSELECT = 0x0;
	VIC2INTSELECT = 0x0;
	VIC3INTSELECT = 0x0;

	// 清VICxADDR
	intc_clearvectaddr();

}

// 保存需要处理的中断的中断处理函数的地址 ,在这里设置中断跳转的函数 每一个中断管理器都有自己的
//跳转地址
void intc_setvectaddr(unsigned long intnum, void (*handler)(unsigned long))
{
	//VIC0
	if(intnum<32)			
	{
		*( (volatile unsigned long *)(VIC0VECTADDR + 4*intnum) ) = (unsigned long)handler;
	}
	//VIC1
	else if(intnum<64) 		
	{
		*( (volatile unsigned long *)(VIC1VECTADDR + 4*(intnum-32)) ) = (unsigned long )handler;
	}
	//VIC2
	else if(intnum<96) 			
	{
		*( (volatile unsigned long *)(VIC2VECTADDR + 4*(intnum-64)) ) = (unsigned long )handler;
	}
	//VIC3
	else	
	{
		*( (volatile unsigned long *)(VIC3VECTADDR + 4*(intnum-96)) ) = (unsigned long)handler;
	}
	return;
}

// 清除需要处理的中断的中断处理函数的地址  
void intc_clearvectaddr(void)
{
	// VICxADDR:当前正在处理的中断的中断处理函数的地址
	VIC0ADDR = 0;
	VIC1ADDR = 0;
	VIC2ADDR = 0;
	VIC3ADDR = 0;
}

// 使能中断  ，设置完以后使能中断
void intc_enable(unsigned long intnum)
{
	unsigned long temp;
	if(intnum<32)
	{
		temp = VIC0INTENABLE;
		temp |= (1<<intnum);
		VIC0INTENABLE = temp;
	}
	else if(intnum<64)
	{
		temp = VIC1INTENABLE;
		temp |= (1<<(intnum-32));
		VIC1INTENABLE = temp;
	}
	else if(intnum<96)
	{
		temp = VIC2INTENABLE;
		temp |= (1<<(intnum-64));
		VIC2INTENABLE = temp;
	}
	else if(intnum<NUM_ALL)
	{
		temp = VIC3INTENABLE;
		temp |= (1<<(intnum-96));
		VIC3INTENABLE = temp;
	}
	// NUM_ALL : enable all interrupt
	else 
	{
		VIC0INTENABLE = 0xFFFFFFFF;
		VIC1INTENABLE = 0xFFFFFFFF;
		VIC2INTENABLE = 0xFFFFFFFF;
		VIC3INTENABLE = 0xFFFFFFFF;
	}

}

// 禁止中断  
void intc_disable(unsigned long intnum)
{
	unsigned long temp;

	if(intnum<32)
	{
		temp = VIC0INTENCLEAR;
		temp |= (1<<intnum);
		VIC0INTENCLEAR = temp;
	}
	else if(intnum<64)
	{
		temp = VIC1INTENCLEAR;
		temp |= (1<<(intnum-32));
		VIC1INTENCLEAR = temp;
	}
	else if(intnum<96)
	{
		temp = VIC2INTENCLEAR;
		temp |= (1<<(intnum-64));
		VIC2INTENCLEAR = temp;
	}
	else if(intnum<NUM_ALL)
	{
		temp = VIC3INTENCLEAR;
		temp |= (1<<(intnum-96));
		VIC3INTENCLEAR = temp;
	}
	// NUM_ALL : disable all interrupt
	else 
	{
		VIC0INTENCLEAR = 0xFFFFFFFF;
		VIC1INTENCLEAR = 0xFFFFFFFF;
		VIC2INTENCLEAR = 0xFFFFFFFF;
		VIC3INTENCLEAR = 0xFFFFFFFF;
	}

	return;
}


// 读中断状态  
unsigned long intc_getvicirqstatus(unsigned long ucontroller)
{
	if(ucontroller == 0)
		return	VIC0IRQSTATUS;
	else if(ucontroller == 1)
		return 	VIC1IRQSTATUS;
	else if(ucontroller == 2)
		return 	VIC2IRQSTATUS;
	else if(ucontroller == 3)
		return 	VIC3IRQSTATUS;
	else
	{}
	return 0;
}



