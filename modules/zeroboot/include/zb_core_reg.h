/*-----------------------------------------------------------------------------
  file    : zb_core_reg.h
  explain : arm core register definition
  wirter  : boggle70@falinux.com
  date    : 2014-04-28

-------------------------------------------------------------------------------*/

#ifndef _ZB_CORE_REG_H_
#define _ZB_CORE_REG_H_

/* Coretex A9 type definition */

struct core_reg_c0 {
	unsigned long c0_0;
	unsigned long c0_1;
	unsigned long c0_2;
	unsigned long c0_0_2;
};

struct core_reg_c1 {
	unsigned long c0_0;
	unsigned long c0_1;
	unsigned long c0_2;

	unsigned long c1_0;
	unsigned long c1_1;
	unsigned long c1_2;
	unsigned long c1_3;
};

struct core_reg_c2 {
	unsigned long c0_0;
	unsigned long c0_1;
	unsigned long c0_2;
};

struct core_reg_c3 {
	unsigned long c0_0;
};

struct core_reg_c4 {
	unsigned long rsv;
};

struct core_reg_c5 {
	unsigned long c0_0;
	unsigned long c0_1;

	unsigned long c1_0;
	unsigned long c1_1;
};

struct core_reg_c6 {
	unsigned long c0_0;
	unsigned long c0_2;
};

struct core_reg_c7 {
	unsigned long c0_4;

	unsigned long c1_0;
	unsigned long c1_6;
	unsigned long c1_7;

	unsigned long c4_0;
};

struct core_reg_c8 {
	unsigned long rsv;
};

struct core_reg_c9 {
	unsigned long c12_0;
	unsigned long c12_1;
	unsigned long c12_2;
	unsigned long c12_3;
	unsigned long c12_4;
	unsigned long c12_5;

	unsigned long c13_0;
	unsigned long c13_1;
	unsigned long c13_2;

	unsigned long c14_0;
	unsigned long c14_1;
	unsigned long c14_2;
};

struct core_reg_c10 {
	unsigned long c0_0;

	unsigned long c2_0;
	unsigned long c2_1;
};

struct core_reg_c11 {
	unsigned long rsv;
};

struct core_reg_c12 {
	unsigned long c0_0;
	unsigned long c0_1;

	unsigned long c1_0;
	unsigned long c1_1;
};

struct core_reg_c13 {
	unsigned long c0_0;
	unsigned long c0_1;
	unsigned long c0_2;
	unsigned long c0_3;
	unsigned long c0_4;
};

struct core_reg_c14 {
	unsigned long rsv;
};

struct core_reg_c15 {
	unsigned long c0_0;

	unsigned long c5_2_5;
	unsigned long c6_2_5;
	unsigned long c7_2_5;
};


// common cp15 register definition
struct core_reg {
	struct core_reg_c0	c0;
	struct core_reg_c1	c1;
	struct core_reg_c2	c2;
	struct core_reg_c3	c3;
	struct core_reg_c4	c4;
	struct core_reg_c5	c5;
	struct core_reg_c6	c6;
	struct core_reg_c7	c7;
	struct core_reg_c8	c8;
	struct core_reg_c9	c9;
	struct core_reg_c10	c10;
	struct core_reg_c11	c11;
	struct core_reg_c12	c12;
	struct core_reg_c13	c13;
	struct core_reg_c14	c14;
	struct core_reg_c15	c15;
};

#endif
