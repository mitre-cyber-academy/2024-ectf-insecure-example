/**
 * @file    main.c
 * @brief   SPI Master Demo
 * @details Shows Master loopback demo for SPI1 (AKA: SPIMSS)
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
#include "board.h"
#include "led.h"
#include "mxc_pins.h"
#include "nvic_table.h"
#include "spi.h"
#include "spimss.h"
#include "uart.h"

/***** Definitions *****/
#define TEST_LEN 100 // Words
#define BLAH (TEST_LEN / MXC_SPIMSS_FIFO_DEPTH) * MXC_SPIMSS_FIFO_DEPTH
#define OFFSET 256 //offsets data for tx_data
#define VALUE 0xFFFF
#define SPI_SPEED 10000 // Bit Rate

// SELECT FROM BELOW
#define SYNC
//#define ASYNC

#define SPIMSS MXC_SPIMSS //SPIMSS peripheral registers pointer
#define SPIMSS_IRQ SPIMSS_IRQn

/***** Globals *****/
uint16_t rxData[TEST_LEN];
uint16_t txData[TEST_LEN];
volatile int spimssFlag;
mxc_spimss_req_t req;

/***** Functions *****/

void spimss_cb(mxc_spimss_req_t *req, int error)
{
    spimssFlag = error;
}

void SPIMSS_IRQHandler(void)
{
    /* Only calling MXC_SPIMSS_HANDLER is necessary for the interrupt handler in most cases.
	 * 		Since a loopback is used in this case, this logic block is necessary to avoid
	 * 		an RX FIFO overrun.
	 */
    uint32_t rxFifoCnt =
        ((SPIMSS->dma & MXC_F_SPIMSS_DMA_RX_FIFO_CNT) >> MXC_F_SPIMSS_DMA_RX_FIFO_CNT_POS);
    if (req.tx_num != 0) {
        if (req.rx_num >= BLAH) {
            while ((rxFifoCnt == 0) || (rxFifoCnt % (TEST_LEN - BLAH))) {
                rxFifoCnt = ((SPIMSS->dma & MXC_F_SPIMSS_DMA_RX_FIFO_CNT) >>
                             MXC_F_SPIMSS_DMA_RX_FIFO_CNT_POS);
            }
        } else {
            while ((rxFifoCnt == 0) || (rxFifoCnt % MXC_SPIMSS_FIFO_DEPTH)) {
                rxFifoCnt = ((SPIMSS->dma & MXC_F_SPIMSS_DMA_RX_FIFO_CNT) >>
                             MXC_F_SPIMSS_DMA_RX_FIFO_CNT_POS);
            }
        }
    }

    MXC_SPIMSS_Handler(SPIMSS);
}

int main(void)
{
    int i, j, k, err, fails = 0;
    uint16_t temp;

    printf("\n\n************** SPIMSS Master Loopback Demo ****************\n");
    printf("This example configures the SPIMSS to send data between the MISO (P0.10) and\n");
    printf("MOSI (P0.11) pins.  Connect these two pins together.  This demo shows SPIMSS\n");
    printf("sending between 1 and 16 bits of data at a time.  During this demo you\n");
    printf("may see junk data printed to the serial port because the console UART\n");
    printf("shares the same pins as the SPIMSS.\n\n");
    while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}

#ifdef ASYNC
    MXC_NVIC_SetVector(SPIMSS_IRQ, SPIMSS_IRQHandler);
    NVIC_EnableIRQ(SPIMSS_IRQ);
#endif

    for (i = 1; i < 17; i++) {
        //Initialize transmit and receive buffers
        for (j = 0; j < TEST_LEN; j++) {
            txData[j] = j + OFFSET;
        }
        memset(rxData, 0x0, TEST_LEN * 2);

        // Configure the peripheral
        if (MXC_SPIMSS_Init(SPIMSS, 0, SPI_SPEED, MAP_A) != 0) {
            Console_Init();
            printf("Error configuring SPI\n");
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
            return E_FAIL;
        }

        req.tx_data = txData;
        req.rx_data = rxData;
        req.len = TEST_LEN;
        req.bits = i;
        req.deass = 1;
        req.tx_num = 0;
        req.rx_num = 0;
        req.callback = spimss_cb;
        spimssFlag = 1;

#ifdef ASYNC
        if ((err = MXC_SPIMSS_MasterTransAsync(SPIMSS, &req)) != E_NO_ERROR) {
            Console_Init();
            printf("SPIMSS Asynchronus Transaction failed with error code : %d", spimssFlag);
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
            return err;
        }
        while (spimssFlag == 1) {}
        if (spimssFlag != 0) {
            Console_Init();
            printf("SPIMSS Asynchronus Transaction failed with error code : %d", spimssFlag);
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
            return E_FAIL;
        }
#endif
#ifdef SYNC
        if ((err = MXC_SPIMSS_MasterTrans(SPIMSS, &req)) != E_NO_ERROR) {
            Console_Init();
            printf("SPIMSS Transaction failed with error code:%d\n", err);
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
            return err;
        }
#endif

        k = OFFSET;
        for (j = 0; j < TEST_LEN; j++) {
            if (req.bits <= 8) {
                if (j < (TEST_LEN / 2)) {
                    temp = (VALUE >> (16 - req.bits) << 8) | (VALUE >> (16 - req.bits));
                    temp &= k;
                    txData[j] = temp;

                } else if (j == (TEST_LEN / 2) && TEST_LEN % 2 == 1) {
                    temp = VALUE >> (16 - req.bits);
                    temp &= k;
                    txData[j] = temp;
                } else {
                    txData[j] = 0x0000;
                }
            } else {
                temp = VALUE >> (16 - req.bits);
                temp &= k;
                txData[j] = temp;
            }
            k++;
        }

        // Compare Sent data vs Received data
        // Printf needs the Uart turned on since they share the same pins
        if (memcmp(rxData, txData, sizeof(txData)) != 0) {
            Console_Init();
            printf("\nError verifying rx_data for data width %d\n\n", i);
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
            fails++;
        } else {
            Console_Init();
            printf("Sent %d bits per transaction\n", i);
            while (MXC_UART_Busy(MXC_UART_GET_UART(CONSOLE_UART))) {}
        }
        MXC_SPIMSS_Shutdown(SPIMSS);
    }

    Console_Init();

    if (fails != 0) {
        printf("\nTest failed!\n");
        return E_FAIL;
    } else {
        printf("\nTest successful!\n");
        LED_Init();
        LED_On(0);
    }

    return E_NO_ERROR;
}
