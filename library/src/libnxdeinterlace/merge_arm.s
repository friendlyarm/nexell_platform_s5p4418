 @*****************************************************************************
 @ merge_arm.S : ARM NEON mean
 @*****************************************************************************
 @ Copyright (C) 2009-2012 Remi Denis-Courmont
 @
 @ This program is free software; you can redistribute it and/or modify
 @ it under the terms of the GNU Lesser General Public License as published by
 @ the Free Software Foundation; either version 2.1 of the License, or
 @ (at your option) any later version.
 @
 @ This program is distributed in the hope that it will be useful,
 @ but WITHOUT ANY WARRANTY; without even the implied warranty of
 @ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 @ GNU Lesser General Public License for more details.
 @
 @ You should have received a copy of the GNU Lesser General Public License
 @ along with this program; if not, write to the Free Software Foundation,
 @ Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 @****************************************************************************/

	.syntax	unified
	.arm
	.arch	armv6
	.fpu	neon
	.text

@#define	DEST	r0
@#define	SRC1	r1
@#define	SRC2	r2
@#define	SIZE	r3

	.align 2
	.global merge8_arm_neon
	.type	merge8_arm_neon, %function
	@ NOTE: Offset and pitch must be multiple of 16-bytes in VLC.
merge8_arm_neon:
	cmp		r3,	#64
	blo		2f
1:
	pld		[r1, #64]
	vld1.u8		{q0-q1},	[r1,:128]!
	pld		[r2, #64]
	vld1.u8		{q8-q9},	[r2,:128]!
	vhadd.u8	q0,	q0,	q8
	sub		r3,	r3,	#64
	vld1.u8		{q2-q3},	[r1,:128]!
	vhadd.u8	q1,	q1,	q9
	vld1.u8		{q10-q11},	[r2,:128]!
	vhadd.u8	q2,	q2,	q10
	cmp		r3,	#64
	vhadd.u8	q3,	q3,	q11
	vst1.u8		{q0-q1},	[r0,:128]!
	vst1.u8		{q2-q3},	[r0,:128]!
	bhs		1b
2:
	cmp		r3,	#32
	blo		3f
	vld1.u8		{q0-q1},	[r1,:128]!
	sub		r3,	r3,	#32
	vld1.u8		{q8-q9},	[r2,:128]!
	vhadd.u8	q0,	q0,	q8
	vhadd.u8	q1,	q1,	q9
	vst1.u8		{q0-q1},	[r0,:128]!
3:
	cmp		r3,	#16
	bxlo		lr
	vld1.u8		{q0},		[r1,:128]!
	sub		r3,	r3,	#16
	vld1.u8		{q8},		[r2,:128]!
	vhadd.u8	q0,	q0,	q8
	vst1.u8		{q0},		[r0,:128]!
	bx		lr

	.align 2
	.global merge16_arm_neon
	.type	merge16_arm_neon, %function
merge16_arm_neon:
	cmp		r3,	#64
	blo		2f
1:
	pld		[r1, #64]
	vld1.u16	{q0-q1},	[r1,:128]!
	pld		[r2, #64]
	vld1.u16	{q8-q9},	[r2,:128]!
	vhadd.u16	q0,	q0,	q8
	sub		r3,	r3,	#64
	vld1.u16	{q2-q3},	[r1,:128]!
	vhadd.u16	q1,	q1,	q9
	vld1.u16	{q10-q11},	[r2,:128]!
	vhadd.u16	q2,	q2,	q10
	cmp		r3,	#64
	vhadd.u16	q3,	q3,	q11
	vst1.u16	{q0-q1},	[r0,:128]!
	vst1.u16	{q2-q3},	[r0,:128]!
	bhs		1b
2:
	cmp		r3,	#32
	blo		3f
	vld1.u16	{q0-q1},	[r1,:128]!
	sub		r3,	r3,	#32
	vld1.u16	{q8-q9},	[r2,:128]!
	vhadd.u16	q0,	q0,	q8
	vhadd.u16	q1,	q1,	q9
	vst1.u16	{q0-q1},	[r0,:128]!
3:
	cmp		r3,	#16
	bxlo		lr
	vld1.u16	{q0},		[r1,:128]!
	sub		r3,	r3,	#16
	vld1.u16	{q8},		[r2,:128]!
	vhadd.u16	q0,	q0,	q8
	vst1.u16	{q0},		[r0,:128]!
	bx		lr

	.align 2
	.global merge8_armv6
	.type	merge8_armv6, %function
merge8_armv6:
	push		{r4-r9,lr}
1:
	pld		[r1, #64]
	ldm		r1!,	{r4-r5}
	pld		[r2, #64]
	ldm		r2!,	{r8-r9}
	subs		r3,	r3,	#16
	uhadd8		r4,	r4,	r8
	ldm		r1!,	{r6-r7}
	uhadd8		r5,	r5,	r9
	ldm		r2!,	{ip,lr}
	uhadd8		r6,	r6,	ip
	stm		r0!,	{r4-r5}
	uhadd8		r7,	r7,	lr
	stm		r0!,	{r6-r7}
	popeq		{r4-r9,pc}
	b		1b

	.align 2
	.global merge16_armv6
	.type	merge16_armv6, %function
merge16_armv6:
	push		{r4-r9,lr}
1:
	pld		[r1, #64]
	ldm		r1!,	{r4-r5}
	pld		[r2, #64]
	ldm		r2!,	{r8-r9}
	subs		r3,	r3,	#16
	uhadd16		r4,	r4,	r8
	ldm		r1!,	{r6-r7}
	uhadd16		r5,	r5,	r9
	ldm		r2!,	{ip,lr}
	uhadd16		r6,	r6,	ip
	stm		r0!,	{r4-r5}
	uhadd16		r7,	r7,	lr
	stm		r0!,	{r6-r7}
	popeq		{r4-r9,pc}
	b		1b
