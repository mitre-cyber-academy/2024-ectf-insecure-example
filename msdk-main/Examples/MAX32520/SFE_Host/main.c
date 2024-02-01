/**
 * @file    main.c
 * @brief   SPI Master Demo
 * @details Shows Master loopback demo for QSPI0
 *          Read the printf() for instructions
 */

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

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "nvic_table.h"
#include "sfe_host.h"
#include "sfe.h"
#include "spi.h"
#include "icc.h"

/* **** Definitions **** */
#define TEST_FLASH_WRITE //1.TEST_RAM_WRITE   2.TEST_FLASH_WRITE

#define TEST_LEN 256

/* **** Globals **** */
uint8_t rx_data[TEST_LEN];

uint8_t tx_data[] = {
    0xf0, 0xC2, 0x63, 0x35, 0x18, 0x1c, 0x26, 0x33, 0xb9, 0xe4, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x0A, 0x23, 0x03, 0x49, 0x40, 0x22, 0x01, 0x3B, 0x0A, 0x60, 0x4A, 0x60, 0xF9, 0xD1, 0x70, 0x47,
    0x1C, 0x80, 0x00, 0x40, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xf1, 0xC2, 0x63, 0x35, 0x18, 0x1c, 0x26, 0x33, 0xb9, 0xe4, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x24, 0x06, 0x19, 0x91, 0x40, 0x22, 0x01, 0x3B, 0x0A, 0x60, 0x4A, 0x60, 0xF9, 0xD1, 0x70, 0x47,
    0x1f, 0x2C, 0x36, 0x53, 0x81, 0xC1, 0x62, 0x33, 0x9b, 0x4e, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x0A, 0x23, 0x03, 0x49, 0x40, 0x22, 0x01, 0x3B, 0x0A, 0x60, 0x4A, 0x60, 0xF9, 0xD1, 0x70, 0x47,
    0xf1, 0xC2, 0x63, 0x35, 0x18, 0x1c, 0x26, 0x33, 0xb9, 0xe4, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x5c, 0x6d, 0x52, 0xfa, 0x4c, 0x17, 0x53, 0xf1, 0x27, 0x02, 0x69, 0x4c, 0x17, 0x12, 0x28, 0x35,
    0x0A, 0x23, 0x03, 0x49, 0x40, 0x22, 0x01, 0x3B, 0x0A, 0x60, 0x4A, 0x60, 0xF9, 0xD1, 0x70, 0x47,
    0x08, 0xa1, 0x85, 0xc6, 0xa7, 0xe8, 0x4a, 0xcb, 0x77, 0xf3, 0x0b, 0x47, 0x32, 0xc7, 0x07, 0xa2,
    0x1f, 0x2C, 0x36, 0x53, 0x81, 0xC1, 0x62, 0x33, 0x9b, 0x4e, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x0A, 0x23, 0x03, 0x49, 0x40, 0x22, 0x01, 0x3B, 0x0A, 0x60, 0x4A, 0x60, 0xF9, 0xD1, 0x70, 0x47,
    0xf1, 0xC2, 0x63, 0x35, 0x18, 0x1c, 0x26, 0x33, 0xb9, 0xe4, 0x72, 0x39, 0xa4, 0x52, 0x29, 0xac,
    0x87, 0xb1, 0xe7, 0x15, 0x2b, 0x7f, 0x06, 0x6c, 0x98, 0xa4, 0xa0, 0x9d, 0x51, 0xb0, 0x97, 0xf4,
    0xb8, 0x8f, 0x19, 0x39, 0xf9, 0xd8, 0xc1, 0x3e, 0x7c, 0xba, 0x28, 0xde, 0x8d, 0x03, 0xd1, 0x33
};

/* ************************************************************************** */
int main(void)
{
    printf("\n***** Serial Flash Emulator SPI Master *****\n");
    printf("\nThis example initializes SFE SPI Host. Coneect\n");
    printf("SPI1 pins to SFE Slave device. You can test RAM\n");
    printf("or Flash write and read loopback\n");

    int ret = E_NO_ERROR;

    // Configure the peripheral
    if (MXC_SPI_Init(MASTER_SPI, 1, 0, 1, 0, MASTER_SPI_SPEED) != E_NO_ERROR) {
        printf("\nError configuring SPI\n");
        return E_UNINITIALIZED;
    }

    if (MXC_SPI_SetDataSize(MASTER_SPI, 8) != E_NO_ERROR) {
        printf("\nError setting Data size\n");
        return E_FAIL;
    }

    // Initialize RX buffer to store data
    memset(rx_data, 0x0, TEST_LEN);

    /*************************************************************************************/

    SFE_Reset();

#ifdef TEST_RAM_WRITE

    printf("\nWriting Data to RAM of SFE slave device and reading the data back\n");

    SFE_4ByteModeEnable();

    SFE_RAMWrite(tx_data, TEST_LEN, RAM_SBA, SPI_WIDTH_01, SFE_4BYTE);

    MXC_Delay(1000000);

    SFE_Read(rx_data, TEST_LEN, RAM_SBA, SPI_WIDTH_01, SFE_4BYTE);

    SFE_4ByteModeDisable();

#endif

#ifdef TEST_FLASH_WRITE

    printf("\nWriting Data to Flash of SFE slave device and reading the data back\n");

    SFE_FlashWrite(NULL, 0, FLASH_SBA, FLASH_PAGE_ERASE, SPI_WIDTH_01, SFE_3BYTE);

    MXC_Delay(1000000);

    SFE_FlashWrite(tx_data, TEST_LEN, FLASH_SBA, FLASH_WRITE, SPI_WIDTH_01, SFE_3BYTE);

    MXC_Delay(1000000);

    SFE_Read(rx_data, TEST_LEN, FLASH_SBA, SPI_WIDTH_01, SFE_3BYTE);

#endif

    if (memcmp(tx_data, rx_data, TEST_LEN) == E_NO_ERROR) {
        printf("\nData Verified\n");
    } else {
        printf("\nData Not Verified\n");
        ret = E_FAIL;
    }

    MXC_SPI_Shutdown(MASTER_SPI);

    return ret;
}