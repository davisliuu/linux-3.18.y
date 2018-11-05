/*
 *  Copyright (c) 2014 Hisilicon Limited.
 *  Copyright (c) 2014 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>

	__CPUINIT

ENTRY(hix5hd2_secondary_startup)
	bl	v7_invalidate_l1
	b	secondary_startup
ENDPROC(hix5hd2_secondary_startup)


ENTRY(hi3519_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3519_secondary_startup)

ENTRY(hi3519_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3519_cpu_resume)

ENTRY(hi3516av200_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3516av200_secondary_startup)

ENTRY(hi3516av200_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3516av200_cpu_resume)

ENTRY(hi3559_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3559_secondary_startup)

ENTRY(hi3559_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3559_cpu_resume)
