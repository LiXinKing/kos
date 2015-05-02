#include <s5pc110.h>
#include <smdkv210single.h>


/*
 *************************************************************************
 *
 * 异常跳转表在这里，包括各种出错和普通和快速中断
	其实终端会跑到先行地址0x000000这个位置去找异常向量表，如果没有找到就会
	跑到0xd0037400这个位置去。。
	因此，这里在lowlevel.S中将地址0x000映射到链接地址也就是0x23e00000
	这个地址恐怕是要更改的，所以映射地址也要随之更改才行
 *
 *************************************************************************
 */

.globl _start
_start: b	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq
_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_not_used:
	.word not_used
_irq:
	.word irq
_fiq:
	.word fiq
/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * setup Memory and board specific bits prior to relocation.
 * relocate armboot to ram
 * setup stack
 *
 *************************************************************************
 */
/*在Makefile中赋值
首先这个值最初是通过make 《配置》生成的，目录是
board/samsung/s5pc110/config.mk下，然后在调用主Makefile的是
通过-DTEXT_BASE 《值即上面文件里的值》来指定这个变量的值
值为 0xc3e0 0000
基地址现留着
*/

/*
这个TEXT_BASE对应于重定位的代码的位置，所谓重定位代码的位置是相对于虚拟地址而言的，就是
cpu看到的地址。而拷贝地址是对应于虚拟地址映射后的物理地址。
这里的例子是TEXT_BASE是链接的时候用-Ttext TEXT_BASE来指定重定位的地址（0xc3e0 0000）
而后面的拷贝到实际DRAM的地址是0x23e0 0000

实际上cpu的操作过程是：
	跳转到一个符号上，那么就会去这个符号的地址，这个地址是程序在链接的时候由相应的LD来指定的
如果重定位的初始的地址为0xc3e0 0000 那么符号的地址（绝对地址）可能为0xc3e0 0100，cpu拿着这个地址去DRAM中
去寻找这个内存单元，中间是要通过mmu进行地址变化，这里的转换就是0xc3e0 0000->0x23e0 0000，那么
实际拿到的是dram中0x23e 0100处的指令，然后执行。这也就解释了为什么代码重定位的地址和实际拷贝的
地址不同的原因。
*/
_TEXT_BASE:
	.word	TEXT_BASE


_TEXT_PHY_BASE:
	.word	CFG_PHY_UBOOT_BASE

/*
 * These are defined in the board-specific linker script.
 这里的变量_bss_start和_end是在u-boot.lds中定义的，后面要将
 这一部分清除，这里需要注意的是，这个如果是取绝对地址的话就是
 ram中的那一部分
 */
.globl _bss_start
_bss_start:
	.word __bss_start

.globl _bss_end
_bss_end:
	.word _end
/*
先不设置IRQ和FIQ的堆栈，下面禁止了
*/

/*
 * the actual reset code,进入管理模式
 */

reset:

	/*
	 * set the cpu to SVC32 mode and IRQ & FIQ disable
	使cpu处于SVC状态禁止IRQ和FIQ
	 */
	msr	cpsr_c, #0xd3		@ I & F disable, Mode: 0x13 - SVC


/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************
 */
         /*
         * we do sys-critical inits only at reboot,
         * not when booting from ram!
         */
cpu_init_crit:
		bl	disable_l2cache
	/*把寄存器都清零了*/
		mov	r0, #0x0	@ 
		mov	r1, #0x0	@ i	
		mov	r3, #0x0
		mov	r4, #0x0
	lp1:
		mov	r2, #0x0	@ j
	lp2:	
		mov	r3, r1, LSL #29		@ r3 = r1(i) <<29
		mov	r4, r2, LSL #6		@ r4 = r2(j) <<6
		orr	r4, r4, #0x2		@ r3 = (i<<29)|(j<<6)|(1<<1)
		orr	r3, r3, r4
		mov	r0, r3			@ r0 = r3
	/*下面这个函数是将数据缓存设置为r0*/
		bl	CoInvalidateDCacheIndex
		add	r2, #0x1		@ r2(j)++
		cmp	r2, #1024		@ r2 < 1024
		bne	lp2			@ jump to lp2
		add	r1, #0x1		@ r1(i)++
		cmp	r1, #8			@ r1(i) < 8
		bne	lp1			@ jump to lp1
	/*设置二级缓存*/
		bl	set_l2cache_auxctrl
		/*使能二级缓存*/
		bl	enable_l2cache
		
		bl	disable_l2cache
	
		bl	set_l2cache_auxctrl_cycle

		bl	enable_l2cache
	
       /*
        * Invalidate L1 I/D
        */
        mov	r0, #0                  @ set up for MCR,这个赋值是为下面一条指令作准备
        mcr	p15, 0, r0, c8, c7, 0   @ invalidate TLBs，使原来的TLB中内容无效
        mcr	p15, 0, r0, c7, c5, 0   @ invalidate icache，使原来的icache中的内容无效

       /*
        * disable MMU stuff and caches，禁止MMU和缓存
		首先前三位是010表示关闭mmu（0位），使能严格的对齐检查（1），一级数据缓存禁止
		12位置1，使能了一级指令缓存
		可以看到TLB和icache（指令缓存）中的数据被invalidate了，所以这里开启了它们的作用
		而Dcache必须要在mmu开启后才能适用使用
		前面CoInvalidateDCacheIndex的内容应该就是清除Dcache缓存的代码，我们去开始mmu的
		地方去开启这个dceche
        */
        mrc	p15, 0, r0, c1, c0, 0
        bic	r0, r0, #0x00002000     @ clear bits 13 (--V-)
        bic	r0, r0, #0x00000007     @ clear bits 2:0 (-CAM)
        orr	r0, r0, #0x00000002     @ set bit 1 (--A-) Align
        orr	r0, r0, #0x00000800     @ set bit 12 (Z---) BTB
        mcr 	p15, 0, r0, c1, c0, 0


        /* Read booting information 
		PRO_ID_BASE=0xE0000000
		在手册中没有找到这个寄存器，但是网上说是启动项寄存器
		读这个就可以知道具体是通过哪里启动的，在开发板中通过
		拨动按钮来控制。
		*/
        ldr	r0, =PRO_ID_BASE
        ldr	r1, [r0,#OMR_OFFSET]
        bic	r2, r1, #0xffffffc1

	/* NAND BOOT */
	cmp	r2, #0x0		@ 512B 4-cycle
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x2		@ 2KB 5-cycle
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x4		@ 4KB 5-cycle	8-bit ECC
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x6		@ 4KB 5-cycle	16-bit ECC
	moveq	r3, #BOOT_NAND


	/* SD/MMC BOOT */
	cmp     r2, #0xc
	moveq   r3, #BOOT_MMCSD	



	/* Uart BOOTONG failed r2=0x10说明出问题了*/
	cmp     r2, #(0x1<<4)
	moveq   r3, #BOOT_SEC_DEV
	/*用户自定义数据寄存器，存了启动选项所代表的值*/
	ldr	r0, =INF_REG_BASE
	str	r3, [r0, #INF_REG3_OFFSET]     

	/*
	 * Go setup Memory and board specific bits prior to relocation.
	 注意的是，现在还是在IRAM中，将堆栈设置在IRAM中尾端
	 iRAM的顶端地址是0xd0037fff
*/
	ldr	sp, =0xd0036000 /* end of sram dedicated to u-boot */
	sub	sp, sp, #12	/* set stack */
	/*下面这个是栈侦，就是只有当调用函数的时候push前一个函数的栈侦进堆栈，调用结束以后再
	pop出来，常常通过这个指针来寻址传递参数，下面的函数应该没有用到这个栈侦否则是会出错的
	*/
	mov	fp, #0
	
	bl	lowlevel_init	/* go setup pll,mux,memory */

	/* get ready to call C functions  SVC模式的堆栈 */
	ldr	sp, =STACK_COPY	/* setup temp stack pointea  0x23e0 0000已经在ram上的设置*/
	sub	sp, sp, #12
	mov	fp, #0			/* no previous frame, so fp=0 */

	/* when we already run in ram, we don't need to relocate U-Boot.
	 * and actually, memory controller must be configured before U-Boot
	 * is running in ram.
	 */
	/* If BL1 was copied from SD/MMC CH2 */
	ldr	r0, =0xD0037488
	ldr	r1, [r0]
	ldr	r2, =0xEB200000
	cmp	r1, r2
	beq     mmcsd_boot
	
	
/*
 这是一个用户自己适用的寄存器，刚才已经将启动方式的值保存在这里了,现在
 取出来用
*/
	ldr	r0, =INF_REG_BASE
	ldr	r1, [r0, #INF_REG3_OFFSET]
	cmp	r1, #BOOT_NAND		/* 0x0 => boot device is nand */
	beq	nand_boot

	cmp     r1, #BOOT_MMCSD
	beq     mmcsd_boot


	cmp     r1, #BOOT_SEC_DEV
	beq     mmcsd_boot
/*
搬运的目的地址是0x23e0 0000，这个地址和sp的地址是一样的，
但是sp是往下走，这个是往高地址走
*/
nand_boot:
	mov	r0, #0x1000
	bl	copy_from_nand
	b	after_copy


mmcsd_boot:
	bl      movi_bl2_copy
	b       after_copy



after_copy:
	/*将内存映射表拷贝到0x20000000的位置吧*/
	
	


@	ldr r0,_mmu_table_base
@	ldr r1,=0x20000000
@	mov r2,#0x21000
@t:	ldr r3,[r0]
@	str r3,[r1]
@	add r0,r0,#4
@	add r1,r1,#4
@	sub r2,r2,#1
@	cmp r2,#0x0
@	bne t
	/*ldr pc,=0x200004e8*/
	/*bl after_test*/

	
	
@	bl after_test
	
	

/*
在smdkv210single.h中定义的,MMU的映射表是在lowlevel_init.S中
*/
#if defined(CONFIG_ENABLE_MMU)
enable_mmu:
	/* enable domain access */
	ldr	r5, =0x5555fff7
	mcr	p15, 0, r5, c3, c0, 0		@load domain access register,先将这个控制域废置
@下面是设置映射的页表。。可以减少映像的大小。。
	ldr r0,_mmu_table_base
	ldr	r4, =CFG_PHY_UBOOT_BASE
	ldr	r5, =0xfff00000
	bic	r0, r0, r5
	orr	r0, r0, r4
	
	
	ldr r1,_mmu_table_1
	ldr	r4, =CFG_PHY_UBOOT_BASE
	ldr	r5, =0xfff00000
	bic	r1, r1, r5
	orr	r1, r1, r4

	mov r2,#64
	bic r3,r1,#0xff
	bic r3,r3,#0x300
	orr r3,r3,#0x21
t:	str r3,[r0]
	add r3,r3,#0x400
	sub r2,r2,#1
	add r0,r0,#4
	cmp r2,#0x0
	bne t
	
	mov r2,#0x4000
	ldr r3,=0x2000003E
l:	str r3,[r1]
	sub r2,r2,#1
	add r3,r3,#0x1000
	add r1,r1,#4
	cmp r2,#0x0
	bne l
	/*
	在这里时Dcache无效，并且在打开mmu的时候同时开启Dcache的作用
	*/
	/* Set the TTB register 
	这里是设置地址转换表的基地址
	mmu
	*/
	ldr	r0, _mmu_table_base
	ldr	r1, =CFG_PHY_UBOOT_BASE
	ldr	r2, =0xfff00000
	bic	r0, r0, r2
	orr	r1, r0, r1
	/*ldr r1,=0x20008000*/
	mcr	p15, 0, r1, c2, c0, 0
	

	/* Enable the MMU */
mmu_on:
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #1
@	bic r0,r0,#0x2000
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop
	nop
#endif
	/*bl after_test*/
/*
config_memory_upper_code这个是有定义的
其中
#define CFG_UBOOT_SIZE		(2*1024*1024)
 Set up the stack						    */

stack_setup:
	ldr	sp,=0x400000		/* upper 128 KiB: relocated uboot   */
@	sub	r0, r0, #CFG_MALLOC_LEN	/* malloc area                      */
@	sub	r0, r0, #CFG_GBL_DATA_SIZE /* bdinfo                        */
@	sub	sp, r0, #12		/* leave 3 words for abort-stack    */

/*
因为这里是取绝对地址，所以清除的是ram上的bss
*/
clear_bss:
	ldr	r0, _bss_start		/* find start of bss segment        */
	ldr	r1, _bss_end		/* stop here                        */
	mov 	r2, #0x00000000		/* clear                            */

clbss_l:
	str	r2, [r0]		/* clear loop...                    */
	add	r0, r0, #4
	cmp	r0, r1
	ble	clbss_l
	/*
	通过绝对地址跳转到ram中的这个函数中
	*/
	/*ldr pc,=0x23e00d64*/
	

@	msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SYS_MOD)
@	ldr sp, =0x2fa00000
@	ldr pc,=0x528
@	ldr r0, =0x00
@	mcr	p15, 0, r0, c12, c0, 0
	ldr	pc, _start_armboot
@	ldr pc,=0xec4
@	bl after_test
@	ldr pc,=0x508
_start_armboot:
	.word start_armboot


/*这个是定义在lowlevel_init.S中*/
#if defined(CONFIG_ENABLE_MMU)
_mmu_table_base:
	.word mmu_table

_mmu_table_1:
	.word mmu_table_1

#endif

/*
 * copy U-Boot to SDRAM and jump to ram (from NAND or OneNAND)
 * r0: size to be compared
 * Load 1'st 2blocks to RAM because U-boot's size is larger than 1block(128k) size
 */
	.globl copy_from_nand
copy_from_nand:
	push	{lr}		/* save return address */

	mov	r9, r0
	
	mov	r9, #0x100		/* Compare about 8KB */
	bl	copy_uboot_to_ram

	tst 	r0, #0x0
	bne	copy_failed
//比较iram中的内容和已经拷贝到的DRAM中的内容是否是相同哦
#if defined(CONFIG_EVT1)
	ldr	r0, =0xd0020000
#else	
	ldr	r0, =0xd0030000
#endif
	ldr	r1, _TEXT_PHY_BASE	/* 0x23e00000 */
1:	ldr	r3, [r0], #4
	ldr	r4, [r1], #4
	teq	r3, r4
	bne	compare_failed	/* not matched */
	subs	r9, r9, #4
	bne	1b

	pop	{pc}		/* all is OK */

copy_failed:
	nop			/* copy from nand failed */
	b	copy_failed

compare_failed:
	nop			/* compare failed */
	b	compare_failed

/*
 * we assume that cache operation is done before. (eg. cleanup_before_linux())
 * actually, we don't need to do anything about cache if not use d-cache in U-Boot
 * So, in this function we clean only MMU. by scsuh
 *
 * void	theLastJump(void *kernel, int arch_num, uint boot_params);
 这个config_enable_mmu是有在smdkv210single.h中定义的
 
 */
/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */
@
@ IRQ stack frame.
@


.equ DISABLE_IRQ,0x80
.equ DISABLE_FIQ,0x40
.equ SYS_MOD,0x1f
.equ IRQ_MOD,0x12
.equ FIQ_MOD,0x11
.equ SVC_MOD,0x13
.equ ABT_MOD,0x17
.equ UND_MOD,0x1b
.equ MOD_MASK,0x1f

.macro	CHANGE_TO_SVC
	msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|SVC_MOD)
.endm

.macro CHANGE_TO_IRQ
	msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|IRQ_MOD)
.endm


.macro CHANGE_TO_UND
	msr     cpsr_c,#(DISABLE_FIQ|DISABLE_IRQ|UND_MOD)
.endm
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp,=0x10000000
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, =0x0ff00000
	.endm


	.macro get_dataabort_stack			@ setup FIQ stack
	ldr	sp, =0x0fe00000
	.endm


	.macro get_prefetchabort_stack			@ setup FIQ stack
	ldr	sp, =0x0fd00000
	.endm

	.macro get_undefined_stack			@ setup undefined stack
	ldr	sp, =0x0fc00000
	.endm

	.macro get_softwareinter_stack			@ setup FIQ stack
	ldr	sp, =0x0fb00000
	.endm
/*
 * exception handlers
 */
	.align	5
undefined_instruction:
	sub r14,r14,#0
	str r14,[r13,#-0x4]
	mrs r14,spsr
	str r14,[r13,#-0x8]
	str r0,[r13,#-0xc]
	mov r0,r13
	CHANGE_TO_SVC
	str r14,[r13,#-0x8]!
	ldr r14,[r0,#-0x4]
	str r14,[r13,#-0x4]
	ldr r14,[r0,#-0x8]
	ldr r0,[r0,#-0xc]
	stmdb r13!,{r0-r3,r14}	@无法处理的出错，这里不切换到内核态进行处理，无法回去的点
	bl exceptionundefins
	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14,pc}^

	.align	5
software_interrupt:		@它的堆栈就是在svc的堆栈
	
	mrc p15,0,ip,c13,c0,0	@存储原来的任务线性地址值
	mov sl,#0
	mcr p15,0,sl,c13,c0,0
	sub r14,r14,#0
	stmfd r13!,{ip}
	stmfd r13!,{r14}
	stmfd r13!,{r14}
	stmfd r13!,{r1,r2}

	ldr r1,[r14,#-4]
	bic r1,r1,#0xFF000000
	ldr r2,=Entrytable
	mrs r14,spsr
	stmfd r13!,{r0-r3,r14}
	mov r14,pc
	ldr pc,[r2,r1,LSL #2]
	ldmfd r13!,{r0-r3,r14}
	ldmfd r13!,{r1,r2}
	msr spsr,r14
	ldmfd r13!,{r14}
	ldmfd r13!,{ip}
	ldmfd r13!,{sl}
	mcr p15,0,sl,c13,c0,0
	mrs sl,spsr
	msr cpsr,sl
	mov pc,ip 

	.align	5
prefetch_abort:

	sub r14,r14,#4
	str r14,[r13,#-0x4]
	mrs r14,spsr
	str r14,[r13,#-0x8]
	str r0,[r13,#-0xc]
	str r9,[r13,#-0x10]
	str r8,[r13,#-0x14]
	mov r0,r13
	CHANGE_TO_SVC
	mrc p15,0,r8,c13,c0,0	@存储原来的任务线性地址值
	mov r9,#0
	mcr p15,0,r9,c13,c0,0
	stmfd r13!,{r8}
	sub sp,sp,#0x4
	stmfd r13!,{r14}
	add sp,sp,#8
	ldr r14,[r0,#-0x4]
	stmfd r13!,{r14}
	ldr r14,[r0,#-0x8]
	ldr r9,[r0,#-0x10]
	ldr r8,[r0,#-0x14]
	ldr r0,[r0,#-0xc]
	sub sp,sp,#4
	stmdb r13!,{r0-r3,r14}

	bl exceptionprefetch

	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14}
	ldmfd r13!,{ip}
@	ldr sl,=current
@	ldr sl,[sl,#12]
@	mov sl,sl,LSL#26
	ldmfd r13!,{sl}
	mcr p15,0,sl,c13,c0,0
	mrs sl,spsr
	msr cpsr,sl
	mov pc,ip

	.align	5
.global data_abort
data_abort:

	sub r14,r14,#8
	str r14,[r13,#-0x4]
	mrs r14,spsr
	str r14,[r13,#-0x8]
	str r0,[r13,#-0xc]
	str r9,[r13,#-0x10]
	str r8,[r13,#-0x14]
	mov r0,r13
	CHANGE_TO_SVC
	mrc p15,0,r8,c13,c0,0	@存储原来的任务线性地址值
	mov r9,#0
	mcr p15,0,r9,c13,c0,0
	stmfd r13!,{r8}
	sub sp,sp,#0x4
	stmfd r13!,{r14}
	add sp,sp,#8
	ldr r14,[r0,#-0x4]
	stmfd r13!,{r14}
	ldr r14,[r0,#-0x8]
	ldr r9,[r0,#-0x10]
	ldr r8,[r0,#-0x14]
	ldr r0,[r0,#-0xc]
	sub sp,sp,#4
	stmdb r13!,{r0-r3,r14}

	bl exceptiondataabort

	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14}
	ldmfd r13!,{ip}
@	ldr sl,=current
@	ldr sl,[sl,#12]
@	mov sl,sl,LSL#26
	ldmfd r13!,{sl}
	mcr p15,0,sl,c13,c0,0
	mrs sl,spsr
	msr cpsr,sl
	mov pc,ip


@	ldmfd和ldmdb好像是一样的
/*
	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14,pc}^
	*/

	.align	5
not_used:
	bl	not_used


	.align	5
.global irq
irq:
	mcr	p15, 0, r0, c7, c5, 0   @ invalidate icache，使原来的icache中的内容无效
    mcr	p15, 0, r0, c8, c7, 0   @ invalidate TLBs，使原来的TLB中内容无效
	sub r14,r14,#4
	str r14,[r13,#-0x4]
	mrs r14,spsr
	str r14,[r13,#-0x8]
	str r0,[r13,#-0xc]
	str r9,[r13,#-0x10]
	str r8,[r13,#-0x14]
	mov r0,r13
	CHANGE_TO_SVC
	mrc p15,0,r8,c13,c0,0	@存储原来的任务线性地址值
	mov r9,#0
	mcr p15,0,r9,c13,c0,0
	stmfd r13!,{r8}
	sub sp,sp,#0x4
	stmfd r13!,{r14}
	add sp,sp,#8
	ldr r14,[r0,#-0x4]
	stmfd r13!,{r14}
	ldr r14,[r0,#-0x8]
	ldr r9,[r0,#-0x10]
	ldr r8,[r0,#-0x14]
	ldr r0,[r0,#-0xc]
	sub sp,sp,#4
	stmdb r13!,{r0-r3,r14}
	
	mov r0,r14
	bl exceptionirq
	
	ldr r0,[r13,#24]
	bl ttest

	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14}
	ldmfd r13!,{ip}
	ldmfd r13!,{sl}
	mcr p15,0,sl,c13,c0,0
	mrs sl,spsr
	msr cpsr,sl
	mov pc,ip



@fiq就暂时不用了，如要适用必须要进行更改
	.align	5
fiq:
	sub r14,r14,#4
/*	str r14,[r13,#-0x4]
	mrs r14,spsr
	str r14,[r13,#-0x8]
	str r0,[r13,#-0xc]
	mov r0,r13
	CHANGE_TO_SVC
	str r14,[r13,#-0x8]!
	ldr r14,[r0,#-0x4]
	str r14,[r13,#-0x4]
	ldr r14,[r0,#-0x8]
	ldr r0,[r0,#-0xc]
	stmdb r13!,{r0-r3,r14}
	bl	exceptionfiq
	ldmia r13!,{r0-r3,r14}
	msr spsr,r14
	ldmfd r13!,{r14,pc}^
*/
	stmfd r13!,{r0-r3,r14}
	bl exceptionfiq
	ldmfd r13!,{r0-r3,pc}^

Entrytable:
#include "entry-common.S"


	.align 5
.global arm_cache_flush
arm_cache_flush:
       mcr     p15, 0, r1, c7, c5, 0           @ invalidate I cache
       mov     pc, lr                          @ back to caller

       .align  5
.global disable_l2cache
/*禁止二级缓存。。*/
disable_l2cache:
	mrc     p15, 0, r0, c1, c0, 1
	bic     r0, r0, #(1<<1)
	mcr     p15, 0, r0, c1, c0, 1
	mov	pc, lr


       .align  5
.global enable_l2cache
enable_l2cache:
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, r0, #(1<<1)
	mcr     p15, 0, r0, c1, c0, 1
	mov     pc, lr


       .align  5
.global set_l2cache_auxctrl_cycle
set_l2cache_auxctrl_cycle:
	mrc 	p15, 1, r0, c9, c0, 2
	bic 	r0, r0, #(0x1<<29)
	bic 	r0, r0, #(0x1<<21)
	bic 	r0, r0, #(0x7<<6)
	bic 	r0, r0, #(0x7<<0)
	mcr 	p15, 1, r0, c9, c0, 2
	mov     pc,lr


       .align  5
.global set_l2cache_auxctrl
set_l2cache_auxctrl:
	mov	r0, #0x0
	mcr     p15, 1, r0, c9, c0, 2
	mov     pc, lr



	.align 5
CoInvalidateDCacheIndex:
	;/* r0 = index */
	mcr     p15, 0, r0, c7, c6, 2
	mov     pc,lr

.global after_test
after_test:
	// 设置GPBCON的bit[0:15]，配置GPB_0/1/2/3引脚为输出功能
	ldr r1, =0xE0200040					
	ldr r0, =0x00001111
	str r0, [r1]
	mov r2, #0x1000
led_blink:
	ldr r1, =0xE0200044 					
	mov r0, #0x0f
	str r0, [r1]
	bl led_blink

	// 延时
	bl delay							

	ldr r1, =0xe0200044				
	mov r0, #0x00
	str r0, [r1]

	// 延时
	bl delay	

//	sub r2, r2, #1
//	cmp r2,#0
	bl led_blink


halt:
	b halt


delay:
	mov r0, #0x4000000
delay_loop:
	sub r0, r0, #1
	cmp r0, #0
	bne delay_loop
	mov pc, lr
