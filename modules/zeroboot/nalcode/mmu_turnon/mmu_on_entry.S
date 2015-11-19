//------------------------------------------------------------------------------
// 파 일 명 : mmu_on_entry.S
// 프로젝트 : zeroboot
// 설    명 : 
// 작 성 자 : 에프에이리눅스(주) boggle70@falinux.com
// 작 성 일 : 2012년 3월 23일
// 저 작 권 : 에프에이리눅스(주)
// 주    의 : 위치 독립적인 코드로만 이루어져 있다.
//			  아래의 코드는 메모리상에 mmu_on_entry 의 시작위치만 
//			  align 되어 있다면 실행이 가능하다.
//------------------------------------------------------------------------------

/* register usage 
 * struct l1 mapping info
 * {
 *		int				used;
 *		unsigned long	fake_l1_value;
 *		unsigned long	fake_l1_ptr;
 *		unsigned long	old_l1_value;
 * }
 *
 * r0 
 * r1 pointer to l1 mapping structure
 * r2 PAGE_OFFSET
 * r3 PHYS_OFFSET
 * r4
 * r5
 * r6
 * r7
 * r8
 * r9
 * r10 backup physical memory l1 table value
 * r11
 * r12
 * r13 virtural address to return
 * r14 lr
 * r15 pc
 */

.text
.globl _mmu_on_entry
_mmu_on_entry:

// s5p4118 don't care mmu on or off
	bl	mmu_off_check_end

	ldr r0, [r1], #4
	tst	r0, #0
	beq	fake_skip

	ldr r0, [r1], #4
	ldr	r1, [r1]
	str r0, [r1]

fake_skip:
//	mov pc, lr

	// confirm irq off
//	mrs	r2, cpsr
//	orr	r2, r2, #1<<6
//	orr	r2, r2, #1<<7
//	msr	cpsr, r2

	b	switch_end

	// check mmu on
	mrc p15, 0, r2, c1, c0, 0		@ CR
	tst	r2, #0x01
	beq	mmu_off_check_end

	adr	r1, switch_pc_table
	ldr	r0, [r1]
	mov	pc, r0

jump_self:
	b	switch_end

switch_pc_table:
.word	jump_self
_mmu_on_entry_table:
.word	_mmu_on_entry

switch_end:

//	ldr     r0,=(0x80862000+0x40)
//	ldr     r1,=0x31
//	str     r1, [r0]
//	b	switch_end
	
#if 1

	mov     r0, #0
	mcr     p15, 0, r0, c13, c0, 0  @ Clear FCSE PID

	// confirm mmu off
.align 5
	mrc p15, 0, r2, c1, c0, 0		@ CR
	bic r2, r2, #1<<2				@ dcache disable
//	bic r2, r2, #1<<11				@ prediction disable
	bic r2, r2, #1<<12				@ icaceh disable
//	mcr p15, 0, r2, c1, c0, 0		@ CR

	bic r2, r2, #0x01				@ MMU disable
	mov	r0, #0
//	mcr p15,0,r0,c8,c7,0      		@ invalidate I + D TLBs
//	mcr	p15,0,r0,c8,c7,0            @ Invalidate whole TLB
	mcr p15,0, r2, c1, c0, 0		@ CR
	dsb
	isb

	nop
	nop
#else
	mrc p15, 0, r2, c1, c0, 0		@ CR
	bic r2, r2, #1<<2				@ dcache disable
	bic r2, r2, #1<<12				@ icaceh disable
	mcr p15, 0, r2, c1, c0, 0		@ CR
	nop
#endif

mmu_off_check_end:
//	ldr     r0,=(ARG_REG_UART_VIRT+0x40)
//	ldr     r0,=(0x21e8000+0x40)
//	ldr     r1,=0x31
//	str     r1, [r0]
//	b       mmu_off_check_end

	// set STACK to current page + 0x1000 - 8
	adr r1, _mmu_on_entry
	add r1, r1, #0x6000
	sub r1, r1, #8
	mov	sp, r1

	bl zeroboot

.align 5
.globl mmu_restore
mmu_restore:
// r4 : original L1
// r5 : TTB base
// r6 : current PC index
// r7 : fake value
#ifdef ARCH_V4
	ldr	r5, =0x41E					@ section flag
#else
	ldr	r5, =0x40E					@ section flag
#endif
	mov r6, pc						@ get pc
	mov	r6, r6, lsr #20				@ make base index
	mov r7, r6, lsl #20				@ set section base 
	orr	r7, r7, r5					@ add secition flag

	mrc p15, 0, r5, c2, c0, 0		@ get TTB Entry
	bic r5, r5, #0x3f00				@ mask TTB
	bic r5, r5, #0x00ff				@ mask TTB
	ldr	r4, [r5, r6, lsl #2]		@ backup original L1
	str	r7, [r5, r6, lsl #2]		@ set fake L1

//	mov r0, r4
//	bl uart_put_hex
//	mov r0, r5
//	bl uart_put_hex
//	mov r0, r6
//	bl uart_put_hex
//	mov r0, r7
//	bl uart_put_hex

//	bic r2, #0x04	@ ensure disable dcache
	b	__mmu_turn_on
	adr	r8, __mmu_turn_on
	add	pc, r8, #0

.align	5
__mmu_turn_on:
	mov	r0, r3                                      
	mcr p15, 0, r2, c1, c0, 0		@ CR           
	mrc	p15, 0, r2, c0, c0, 0		@ read id reg   
	mov	r2, r2                                      
	mov	r2, r1                                      
	mov	pc, lr                                      

