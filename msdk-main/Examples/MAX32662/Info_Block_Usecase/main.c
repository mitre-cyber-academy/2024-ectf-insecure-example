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

/**
 * @file    main.c
 * @brief   Info Block Read/Write Example
 * @details This example demonstrate how to read/write data from MAX32662 info memory
 *
 */

/***** Includes *****/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "mxc_device.h"
#include "flc.h"

/***** Definitions *****/
/*
 *  Attention if you set WITH_WRITE_TEST flag
 *  This example will search Info memory and try to find 0xffffffff data
 *  If it could be found 4 test bytes will be written in that address.
 */
#define WITH_WRITE_TEST 0 // set it to test write test

// Memory Address
// User infoblock area is Infoblock 1
// Write-only area of Infoblock 1: 0x10802000 - 0x10802FFF
// R/W area of Infoblock 1: 0x10803000 - 0x10803FFF
#define READABLE_AREA_OFFSET 0x1000
#define INFO_MEM_READABLE_AREA (MXC_INFO1_MEM_BASE + READABLE_AREA_OFFSET)
#define INFO_MEM_READABLE_AREA_SIZE (MXC_INFO1_MEM_SIZE - READABLE_AREA_OFFSET)

#define INFO_MEM_USER_AREA MXC_INFO1_MEM_BASE
#define INFO_MEM_USER_AREA_SIZE MXC_INFO1_MEM_SIZE

/***** Static Functions *****/
static void dump_section(unsigned int address, unsigned int length)
{
    unsigned int i;
    volatile uint32_t *addr = (uint32_t *)address;

    length /= 4; // on each loop print 4 bytes

    for (i = 0; i < length; i++) {
        if (!(i % 4)) {
            printf("\n0x%08x:", (unsigned int)addr);
        }

        // add extra space
        if (!(i % 2)) {
            printf("   ");
        }

        printf(" %08x", *addr);
        addr++;
    }
}

#if WITH_WRITE_TEST
static int write_test(void)
{
    int ret = 0;
    uint32_t test_val[] = { 0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00 };
    volatile uint32_t *addr = (uint32_t *)INFO_MEM_READABLE_AREA;
    volatile uint32_t *end_addr =
        (uint32_t *)(INFO_MEM_READABLE_AREA + INFO_MEM_READABLE_AREA_SIZE);

    // find free slot
    while (addr < end_addr) {
        if (*addr == 0xffffffff) {
            printf("\n\nFree Addr: 0x%X\n", (uint32_t)addr);
            break;
        }
        addr += 4; // align 128bit, 4bytes*4 = 16bytes
    }

    if (addr >= end_addr) {
        ret = -1; // means free slot not found
        printf("\nFree Index Not Found in OTP\n");
    }

    if (ret == 0) {
        ret = MXC_FLC_Write((uint32_t)addr, sizeof(test_val), test_val);
        if (ret) {
            printf("FLC Write Error: %d\n", ret);
        }
    }

    if (ret == 0) {
        /* Dump user section */
        printf("\n\n***** After Write OTP Section *****\n");
        dump_section((unsigned int)addr, 32);
    }

    return ret;
}
#endif

//******************************************************************************
int main(void)
{
    printf("\n\n***** Info Memory Read/Write Example *****\n");
    printf("***** This example demonstrates how you can read/write Info memory *****\n");

    /* Dump user section */
    printf("\n\n***** USER READABLE AREA *****\n");
    dump_section(INFO_MEM_READABLE_AREA, INFO_MEM_READABLE_AREA_SIZE);

#if WITH_WRITE_TEST
    // run write test
    write_test();
#endif

    printf("\n\nExample End\n");

    return 0;
}