.global move_to_usr
move_to_usr:
	mcr	p15, 0, r0, c7, c5, 0   @ invalidate icache，使原来的icache中的内容无效
    mcr	p15, 0, r0, c8, c7, 0   @ invalidate TLBs，使原来的TLB中内容无效
	mov r4,lr
	mov sp,r0	@SVC的堆栈
	msr     CPSR,#(0x80|0x40|0x12)	@设置IRQ的堆栈
	mov sp,r2
	msr     CPSR,#(0x80|0x40|0x11)	@设置FIQ的堆栈
	mov sp,r2
	msr     CPSR,#(0x80|0x40|0x17)	@设置ABT的堆栈，看起来pre和data是共用一个堆栈的
	mov sp,r2
	msr     CPSR,#(0x80|0x40|0x1b)	@设置UND的堆栈
	mov sp,r2
	mrs r0,CPSR
	bic r0,r0,#0x8F	@切换到用户模式同时开始IRQ中断。。
	msr CPSR,r0
	mov sp,r1
	mov pc, r4

.global switch_to
switch_to:
	
	stmfd	sp!, {r0 - r9, fp, lr}		@ Store most regs on stack
	ldr r4,[r0,#12]
	ldr r5,[r1,#12]
	cmp r4,r5
	beq ret
	ldr r4,=current
	ldr r6,=task
	ldr r6,[r6,r5,LSL#2]
	str r6,[r4]    @更换current的值
	mrs	ip, cpsr
	stmfd	sp!, {ip}			@ Save cpsr_SVC
	str	sp, [r0]		@ Save sp_SVC
	ldr	sp, [r1]		@ Get saved sp_SVC
	ldmfd	sp!, {ip}
	msr	spsr, ip			@ Save tasks CPSR into SPSR for this return
	
	ldr r0,[r1,#16]
	cmp r0,#1
	beq forkout	@新任务的返回
	
	ldmfd sp!,{r0 - r9 , fp , pc}^


forkout:
	mov r0,#0
	str r0,[r1,#16]  @将新任务标志清零
	ldr ip,[r1,#12]
	mov ip,ip,LSL#26
	ldr r0,[sp,#44]
	bl ttest
	ldmfd	sp!, {r0 - r9, fp}	@ Load all regs saved previousl
	ldmfd sp!,{sl}
	mcr p15,0,ip,c13,c0,0
	mrs ip,spsr
	msr cpsr,ip
	mov pc,sl
ret:
	ldmfd	sp!, {r0 - r9, fp, pc}	
.global sys_fork
sys_fork:
	stmfd r13!,{r0-r3,r14}
	bl find_empty_process
	cmp r0,#-1
	beq 1f
	bl copy_process

	mov ip,r0
	mov r1,sp
	ldr r2,[r1,#52] 
	stmfd ip!,{r2}	@压入到设置好的堆栈中
	mov r0,#0			@使r0到r9完全相同
	ldr r2,[r1,#44]
	ldr r1,[r1,#40]
	stmfd ip!,{r0 - r9,fp}
	mov r0,#1
	mov r1,sp
	str r0,[r1,#20]		@fork的父进程返回值设置为1
	ldr r2,[r1,#36]
	stmfd ip!,{r2}
	ldmfd r13!,{r0-r3,pc}

1:
	ldmfd r13!,{r0-r3,pc}



