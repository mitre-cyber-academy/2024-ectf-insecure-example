/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 * (now owned by Analog Devices, Inc.),
 * Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 * is proprietary to Analog Devices, Inc. and its licensors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "max32675.h"
#include "gcr_regs.h"
#include "mxc_sys.h"
#include "pwrseq_regs.h"

extern void (*const __vector_table[])(void);

extern void (*const __isr_vector[])(void);

uint32_t SystemCoreClock = HIRC_FREQ;

/*
The libc implementation from GCC 11+ depends on _getpid and _kill in some places.
There is no concept of processes/PIDs in the baremetal PeriphDrivers, therefore
we implement stub functions that return an error code to resolve linker warnings.
*/
int _getpid(void)
{
    return E_NOT_SUPPORTED;
}

int _kill(void)
{
    return E_NOT_SUPPORTED;
}

__weak void SystemCoreClockUpdate(void)
{
    uint32_t base_freq, div, clk_src;

    // Get the clock source and frequency
    clk_src = (MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_SYSCLK_SEL);
    switch (clk_src) {
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_EXTCLK:
        base_freq = EXTCLK_FREQ;
        break;
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_ERFO:
        base_freq = ERFO_FREQ;
        break;
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_INRO:
        base_freq = INRO_FREQ;
        break;
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_IPO:
        base_freq = IPO_FREQ;
        break;
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_IBRO:
        base_freq = IBRO_FREQ;
        break;
    case MXC_S_GCR_CLKCTRL_SYSCLK_SEL_ERTCO:
        base_freq = ERTCO_FREQ;
        break;
    default:
        // Codes 001 and 111 are reserved.
        // This code should never execute, however, initialize to safe value.
        base_freq = HIRC_FREQ;
        break;
    }
    // Get the clock divider
    if (clk_src == MXC_S_GCR_CLKCTRL_SYSCLK_SEL_IPO) {
        uint32_t ovr = (MXC_PWRSEQ->lpcn & MXC_F_PWRSEQ_LPCN_OVR);
        switch (ovr) {
        case MXC_S_PWRSEQ_LPCN_OVR_0_9V:
            base_freq = base_freq >> 3;
            break;
        case MXC_S_PWRSEQ_LPCN_OVR_1_0V:
            base_freq = base_freq >> 1;
            break;
        case MXC_S_PWRSEQ_LPCN_OVR_1_1V:
        default:
            /* Nothing to do here.
                OVR = 1.1V means the clock runs full speed. */
            break;
        }

        base_freq = base_freq >> ((MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_IPO_DIV) >>
                                  MXC_F_GCR_CLKCTRL_IPO_DIV_POS);
    }
    div = (MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_SYSCLK_DIV) >> MXC_F_GCR_CLKCTRL_SYSCLK_DIV_POS;

    SystemCoreClock = base_freq >> div;
}

/* This function is called before C runtime initialization and can be
 * implemented by the application for early initializations. If a value other
 * than '0' is returned, the C runtime initialization will be skipped.
 *
 * You may over-ride this function in your program by defining a custom 
 *  PreInit(), but care should be taken to reproduce the initialization steps
 *  or a non-functional system may result.
 */
__weak int PreInit(void)
{
    /* Do nothing */
    return 0;
}

/* This function can be implemented by the application to initialize the board */
__weak int Board_Init(void)
{
    /* Do nothing */
    return 0;
}

/* This function is called just before control is transferred to main().
 *
 * You may over-ride this function in your program by defining a custom 
 *  SystemInit(), but care should be taken to reproduce the initialization
 *  steps or a non-functional system may result.
 */
__weak void SystemInit(void)
{
    /* Configure the interrupt controller to use the application vector table in */
    /* the application space */
#if defined(__CC_ARM) || defined(__GNUC__)
    /* IAR sets the VTOR pointer incorrectly and causes stack corruption */
    SCB->VTOR = (uint32_t)__isr_vector;
#endif /* __CC_ARM || __GNUC__ */

#if defined __ICCARM__
    SCB->VTOR = (uint32_t)__vector_table;
#endif

    /* Make sure interrupts are enabled. */
    __enable_irq();

    /* Enable FPU on Cortex-M4, which occupies coprocessor slots 10 & 11 */
    /* Grant full access, per "Table B3-24 CPACR bit assignments". */
    /* DDI0403D "ARMv7-M Architecture Reference Manual" */
    SCB->CPACR |= SCB_CPACR_CP10_Msk | SCB_CPACR_CP11_Msk;
    __DSB();
    __ISB();

    /* Change system clock source to the main high-speed clock */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO0);
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO1);

    Board_Init();
}

#if defined(__CC_ARM)
/* Global variable initialization does not occur until post scatterload in Keil tools.*/

/* External function called after our post scatterload function implementation. */
extern void $Super$$__main_after_scatterload(void);

/**
 * @brief   Initialization function for SystemCoreClock and Board_Init.
 * @details $Sub$$__main_after_scatterload is called during system startup in the Keil
 *          toolset. Global variable and static variable space must be set up by the compiler
 *          prior to using these memory spaces. Setting up the SystemCoreClock and Board_Init
 *          require global memory for variable storage and are called from this function in
 *          the Keil tool chain.
 */
void $Sub$$__main_after_scatterload(void)
{
    SystemInit();
    $Super$$__main_after_scatterload();
    while (1) {}
}
#endif /* __CC_ARM */