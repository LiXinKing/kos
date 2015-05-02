#include <smdkv210single.h>
#include <s5pc110.h>
_TEXT_BASE:
	.word	TEXT_BASE

	.globl lowlevel_init
lowlevel_init:
	/*保存回去的地址*/
	push	{lr}

	/* set GPD0(GPD0) to disenable backlight
	这个不对，GPD0接的lcd的背光灯
	这个貌似是让背光灯闪一下
	*/
	@ GPIO setting for backlight
	ldr	r0, =ELFIN_GPIO_BASE
	ldr	r1, =0x1
	str   	r1, [r0, #GPD0CON_OFFSET]

	ldr	r1, =0x0
	str   	r1, [r0, #GPD0DAT_OFFSET]


	/* Disable Watchdog 关闭开门狗*/
	ldr	r0, =ELFIN_WATCHDOG_BASE	/* 0xE2700000 */
	mov	r1, #0
	str	r1, [r0]

	/* when we already run in ram, we don't need to relocate U-Boot.
	 * and actually, memory controller must be configured before U-Boot
	 * is running in ram.
	 */
	ldr	r0, =0xff000fff
	bic	r1, pc, r0		/* r0 <- current base addr of code */
	ldr	r2, _TEXT_BASE		/* r1 <- original base addr in ram */
	bic	r2, r2, r0		/* r0 <- current base addr of code */
	cmp     r1, r2                  /* compare r0, r1                  */
	beq     1f			/* r0 == r1 then skip sdram init   */
/*如果代码已经在ram中了，下面两个函数就不会执行了。。这种情况好像不会出现*/
	/* init PMIC chip */
	/*bl PMIC_InitIp*/

	/* init system clock 
		这里初始化整体时钟以后就初始话了
		onenand和dmc0
	*/

	bl system_clock_init

	/* Memory initialize */

	bl mem_ctrl_asm_init
	
1:
	/* for UART */
	bl uart_asm_init

	bl tzpc_init

#if defined(CONFIG_NAND)
	/* simple init for NAND */
	bl nand_asm_init
#endif


	/* Print 'K' 
	刚才printf了‘O’，组成一个OK
	*/
	ldr	r0, =ELFIN_UART_CONSOLE_BASE
	/*ldr	r1, =0x4b4b4b4b*/
	ldr	r1, =0x4b
	str	r1, [r0, #UTXH_OFFSET]
	pop	{pc}



/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:

	ldr	r0, =ELFIN_CLOCK_POWER_BASE	@0xe0100000

	/* Set Mux to FIN 
	将CLK_SRC0_OFFSET的值都设置为0，这是一个始终输入源的选择：0xE0100200
	后面是设置这个锁定的时间
	
	*/
	ldr	r1, =0x0
	str	r1, [r0, #CLK_SRC0_OFFSET]
/*#define APLL_LOCKTIME_VAL	0x2cf
#define APLL_LOCK_OFFSET		0x00
lock time设定成0x2cf是足够大了
*/
	ldr	r1,	=APLL_LOCKTIME_VAL
	str	r1,	[r0, #APLL_LOCK_OFFSET]


	/* Disable PLL */
#if defined(CONFIG_CHECK_MPLL_LOCK)
retryloop:
#endif
/*#define APLL_CON0_OFFSET		0x100
#define MPLL_CON_OFFSET			0x108
#define CLK_DIV0_OFFSET			0x300
过程是现将APLL和MPLL清零，然后赋值这两个控制寄存器主要设置
时钟转移过此中的分频等参数，然后CLK_DIV这个参数也是设置一些分频的
系数

#define APLL_VAL	set_pll(PLL_MDIV_667, PLL_PDIV_667, PLL_SDIV_667)
#define MPLL_VAL	((Startup_EPLLSTOP<<24)|(Startup_MDIV<<14)|(Startup_PDIV<<5)|(Startup_SDIV))
#define VPLL_VAL	set_pll(EPLL_LOCKED, VPLL_ENABLE_LOCK_DET, VPLL_MDIV, VPLL_PDIV, VPLL_SDIV)
 设置了这三个主要的时钟源，EPLL没有设置
*/
	ldr	r1, =0x0
	str	r1, [r0, #APLL_CON0_OFFSET]
	ldr	r1, =0x0
	str	r1, [r0, #MPLL_CON_OFFSET]

	ldr   	r1, [r0, #CLK_DIV0_OFFSET]
	ldr	r2, =CLK_DIV0_MASK
	bic	r1, r1, r2

	ldr	r2, =CLK_DIV0_VAL
	orr	r1, r1, r2
	str	r1, [r0, #CLK_DIV0_OFFSET]

	ldr	r1, =APLL_VAL
	str	r1, [r0, #APLL_CON0_OFFSET]

	ldr	r1, =MPLL_VAL
	str	r1, [r0, #MPLL_CON_OFFSET]

	ldr	r1, =VPLL_VAL
	str	r1, [r0, #VPLL_CON_OFFSET]
/*
#define APLL_CON1_OFFSET		0x104
#define AFC_ON		0x00000000
AFC能够为输出频率选择一个合适的VCO，更快的频率锁定
时间
*/
#if defined(CONFIG_EVT1)
	ldr	r1, =AFC_ON
	str	r1, [r0, # APLL_CON1_OFFSET]
#endif
/*这里是一个延迟把*/
	mov	r1, #0x10000
1:	subs	r1, r1, #1
	bne	1b
/*
	
*/
#if defined(CONFIG_CHECK_MPLL_LOCK)
	/* MPLL software workaround */
	ldr	r1, [r0, #MPLL_CON_OFFSET]
	orr     r1, r1, #(1<<28)
	str	r1, [r0, #MPLL_CON_OFFSET]

	mov	r1, #0x100
1:	subs	r1, r1, #1
	bne	1b
/*
读第29位看这个MPLL的时钟是否已经处在lock状态
*/
	ldr	r1, [r0, #MPLL_CON_OFFSET]
	and	r1, r1, #(1<<29)
	cmp	r1, #(1<<29)
	bne 	retryloop

#endif
/*
这里是重新设置时钟选择寄存器
*/
	ldr	r1, [r0, #CLK_SRC0_OFFSET]
	ldr	r2, =0x10001111
	orr	r1, r1, r2
	str	r1, [r0, #CLK_SRC0_OFFSET]


#if defined (CONFIG_MCP_SINGLE)
/*
这里是适用默认的时钟源,这个是AUDIO的时钟值
*/
	/* CLK_DIV6 */
	ldr	r1, [r0, #CLK_DIV6_OFFSET]
	bic	r1, r1, #(0x7<<12)	@; ONENAND_RATIO: 0
	str	r1, [r0, #CLK_DIV6_OFFSET]

#endif	

	mov	pc, lr


/*
 * uart_asm_init: Initialize UART in asm mode, 115200bps fixed.
 * void uart_asm_init(void)
 */
uart_asm_init:

	/* set GPIO(GPA) to enable UART */
	@ GPIO setting for UART
	ldr	r0, =ELFIN_GPIO_BASE
	ldr	r1, =0x22222222
	str   	r1, [r0, #GPA0CON_OFFSET]

	ldr     r1, =0xabcdef11
	str     r1, [r0, #GPA1CON_OFFSET]

/*
将好UF和UM两个寄存器清零，后面的UL是设置数据位和停止位的
相当于没有用FIFO，而且无流控
*/
	ldr	r0, =ELFIN_UART_CONSOLE_BASE		@0xE2900000
	mov	r1, #0x0
	str	r1, [r0, #UFCON_OFFSET]
	str	r1, [r0, #UMCON_OFFSET]

	mov	r1, #0x3
	str	r1, [r0, #ULCON_OFFSET]
/*

#define UCON_OFFSET			0x04
#define UART_UBRDIV_VAL		35
#define UART_UDIVSLOT_VAL	0x0

对将uart设置成中断发送和接受的模式，其他的和逻辑程序相似包括时钟源的选择（con设置）
下面对UART_UBRDIV_VAL的设置是和逻辑程序一样的，这个寄存器主要是对时钟进行分频
下面这个寄存器UART_UDIVSLOT_VAL应该改为2比较准确一点
*/
	ldr	r1, =0x3c5
	str	r1, [r0, #UCON_OFFSET]

	ldr	r1, =UART_UBRDIV_VAL
	str	r1, [r0, #UBRDIV_OFFSET]

	ldr	r1, =UART_UDIVSLOT_VAL
	str	r1, [r0, #UDIVSLOT_OFFSET]
/*
往里写的话就是发送,这是个O字符而不是零，而且值发送了一次，因为每次只能发送一个字符
*/
	ldr	r1, =0x4f4f4f4f
	str	r1, [r0, #UTXH_OFFSET]		@'O'
	mov pc, lr



nand_asm_init:

	/* Setting GPIO for NAND */
	/* This setting is NAND initialze code at booting time in iROM. */

	ldr	r0, =ELFIN_GPIO_BASE
	/*
	bic是位清零指令，下面的bic指令是然清除r1中8～11位然后将r1重新扶植给r1
	orr指令是将r1寄存器的值和0x300相或然后将r1的值重新赋值给r1，这么下来相当于
	将8～11位中的8和9位置1其他两位置0，相当于设定MP01中的一个CSn0位。
	*/
	ldr	r1, [r0, #MP01CON_OFFSET]
	bic	r1, r1, #(0xf<<8)
	bic	r1, r1, #(0xf<<12)
	bic	r1, r1, #(0xf<<16)
	bic	r1, r1, #(0xf<<20)

	orr	r1, r1, #(0x3<<8)
	orr	r1, r1, #(0x3<<12)
	orr	r1, r1, #(0x3<<16)
	orr	r1, r1, #(0x3<<20)

	str	r1, [r0, #MP01CON_OFFSET]
/*
将PUD寄存器对应的3～4位清零，表示pull-up/down disabled，关于上拉寄存器的作用在
linux移植文档中有说明，为了排除上拉的映像，这里将上拉都禁止了
*/
	ldr	r1, [r0, #MP01PUD_OFFSET]
	bic	r1, r1, #(0x3<<4)
	str	r1, [r0, #MP01PUD_OFFSET]
/*
MP03这里主要是设置fwen，fren，fcle，fale和rbn0～rbn3的这里应该不需要改了，并在pud寄存器中将
所有的上拉选项禁止掉
*/
	ldr	r1, [r0, #MP03CON_OFFSET]
	bic	r1, r1, #0xFFFFFF
	ldr	r2, =0x22222222
	orr	r1, r1, r2
	str	r1, [r0, #MP03CON_OFFSET]

	ldr	r1, [r0, #MP03PUD_OFFSET]
	ldr	r2, =0xffff
	bic	r1, r1, r2
	str	r1, [r0, #MP03PUD_OFFSET]
/*
这里是设置nand管理的寄存器的地方
#define NFCONF_VAL	(1<<12)|(3<<8)|(1<<4)|(0<<3)|(0<<2)|(1<<1)|(0<<0)       
#define NFCONT_VAL	(0<<18)|(0<<17)|(0<<16)|(0<<10)|(0<<9)|(0<<8)|(0<<7)|(0<<6)|(0x3<<1)|(1<<0)
可以看到的是原来的配置不太对后面要将r1和r2相或然后得到的只才是最终的配置
后面设置了两次nfconf。。这是闹哪样！！
以最后一次为准的话
第一次配置完以后得到的应该就是NFCONF_val的值。第二次以后还是0x7776
这个设置绝对是有问题的，每个page的大小不是512byte！！

*/
	ldr	r0, =ELFIN_NAND_BASE


	ldr	r2, =NFCONF_VAL
	str r2, [r0, #NFCONF_OFFSET]

	ldr	r2, =NFCONT_VAL
	str	r2, [r0, #NFCONT_OFFSET]


	mov	pc, lr

/*
 * Setting TZPC[TrustZone Protection Controller]
#define ELFIN_TZPC0_BASE		0xF1500000
根据文本上的解释，这个是用于指定安全ram的大小，要保证这段内存的安全a？
全都设置成默认值没有安全区域
 */
tzpc_init:

	ldr	r0, =ELFIN_TZPC0_BASE
 	mov	r1, #0x0
 	str	r1, [r0]
 	mov	r1, #0xff
 	str	r1, [r0, #TZPC_DECPROT0SET_OFFSET]
 	str	r1, [r0, #TZPC_DECPROT1SET_OFFSET]
	str	r1, [r0, #TZPC_DECPROT2SET_OFFSET] 	

 	ldr 	r0, =ELFIN_TZPC1_BASE
 	str	r1, [r0, #TZPC_DECPROT0SET_OFFSET]
 	str	r1, [r0, #TZPC_DECPROT1SET_OFFSET]
	str	r1, [r0, #TZPC_DECPROT2SET_OFFSET] 	

 	ldr	r0, =ELFIN_TZPC2_BASE
 	str	r1, [r0, #TZPC_DECPROT0SET_OFFSET]
 	str	r1, [r0, #TZPC_DECPROT1SET_OFFSET]
	str	r1, [r0, #TZPC_DECPROT2SET_OFFSET]
	str	r1, [r0, #TZPC_DECPROT3SET_OFFSET] 

 	ldr	r0, =ELFIN_TZPC3_BASE
 	str	r1, [r0, #TZPC_DECPROT0SET_OFFSET]
 	str	r1, [r0, #TZPC_DECPROT1SET_OFFSET]
	str	r1, [r0, #TZPC_DECPROT2SET_OFFSET] 	

 	mov	pc, lr


/*是否设置了MMU？？*/
#ifdef CONFIG_ENABLE_MMU

/*
 * MMU Table for SMDKC110
 * 0x0000_0000 -- 0xBFFF_FFFF => Not Allowed
 * 0xB000_0000 -- 0xB7FF_FFFF => A:0xB000_0000 -- 0xB7FF_FFFF
 * 0xC000_0000 -- 0xC7FF_FFFF => A:0x3000_0000 -- 0x37FF_FFFF
 * 0xC800_0000 -- 0xDFFF_FFFF => Not Allowed
 * 0xE000_0000 -- 0xFFFF_FFFF => A:0xE000_0000 -- 0XFFFF_FFFF
 */

.macro FL_SECTION_ENTRY1 base,ap,d,c,b
	.word (\base << 20) | (\ap << 10) | \
	      (\d << 5) | (1<<4) | (\c << 3) | (\b << 2) | (1<<1)
.endm
	/* form a first-level section entry */
.macro FL_SECTION_ENTRY base,d
	.word (\base << 10) |  \
	      (\d << 5)  | (1<<0)
.endm

.macro BASE_ADDR base,c,b
	.word (\base << 12) |  \
	       (\c<<3) | (\b << 2) | (1<<1) | (1<<0)
.endm


.section .mmudata, "a"
	.align 14
	// the following alignment creates the mmu table at address 0x4000.这里设置为2的14次方对齐
	//是有于处理器中相关寄存器的是取前面18位为基地址的，地位会被用来作控制为，这样对齐才不会
	//影响低位的数据
	.globl mmu_table
mmu_table:

	@.set __base,0x200
	@.rept 0x40
	@FL_SECTION_ENTRY1 __base,3,0,0,0
	@.set __base,__base+1
	@.endr
	.rept 0x40
	.word 0x00000000
	.endr
	
	.rept 0x200 - 0x40
	.word 0x00000000
	.endr


@	.set __base,0x200040>>2
@	// 1:1 mapping for debugging with non-cacheable
@	.rept 0x300 - 0x200
@	FL_SECTION_ENTRY __base,0
@	.set __base,__base+1
@	.endr


	.rept 0xd00 - 0x200
	.word 0x00000000
	.endr


@	.set __base,0x200440>>2
@	// 1:1 mapping for debugging with non-cacheable
@	.rept 0xd00 - 0xc00
@	FL_SECTION_ENTRY __base,10
@	.set __base,__base+1
@	.endr

	.set __base,0xd00
	// 1:1 mapping for debugging with non-cacheable
	.rept 0x1000 - 0xd00
	FL_SECTION_ENTRY1 __base,3,0,0,0
	.set __base,__base+1
	.endr

	
@	.set __base,0xe00
@	// 1:1 mapping for debugging with non-cacheable
@	.rept 0xf00 - 0xe00
@	FL_SECTION_ENTRY1 __base,3,0,0,0
@	.set __base,__base+1
	.endr

@	.set __base,0xf00
@	// 1:1 mapping for debugging with non-cacheable
@	.rept 0x1000 - 0xf00
@	FL_SECTION_ENTRY1 __base,3,0,0,0
@	.set __base,__base+1
@	.endr


	.align 12
	// the following alignment creates the mmu table at address 0x4000.这里设置为2的14次方对齐
	//是有于处理器中相关寄存器的是取前面18位为基地址的，地位会被用来作控制为，这样对齐才不会
	//影响低位的数据
	.globl mmu_table_1
mmu_table_1:
@	.set __base,0x20000
	// Access for iRAM
@	.rept 0x10000
@	BASE_ADDR __base,1,1
@	.set __base,__base+1
@	.endr

	.rept 0x4000
	.word 0x00000000
	.endr

	@.set __base,0xc0000
	// Access for iRAM
	@.rept 0x20000 - 0x10000 
	@BASE_ADDR __base,1,1
	@.set __base,__base+1
	@.endr

@	//.word 0x2F00002F
@	// Access for iRAM
@	.rept 0x20000 - 0x10000 
@	.word 0x00000000
@	.endr
#endif

