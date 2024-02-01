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
 * @brief   UART!
 * @details This example demonstrates the UART Loopback Test.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "led.h"
#include "board.h"
#include "mxc_delay.h"
#include "uart.h"
#include "dma.h"
#include "nvic_table.h"

/***** Definitions *****/
//#define DMA

#define UART_BAUD 115200
#define BUFF_SIZE 1024

#define READING_UART MXC_UART1
#define READ_IDX MXC_UART_GET_IDX(MXC_UART1)
#define WRITING_UART MXC_UART0
#define WRITE_IDX MXC_UART_GET_IDX(MXC_UART0)

/***** Globals *****/
volatile int READ_FLAG;
volatile int DMA_FLAG;
int first_rx = 1;

/***** Functions *****/
#ifdef DMA
void DMA_Handler(void)
{
    MXC_DMA_Handler();
    DMA_FLAG = 0;
}
#else
void UART_Handler(void)
{
    MXC_UART_AsyncHandler(READING_UART);
}
#endif

void readCallback(mxc_uart_req_t *req, int error)
{
    READ_FLAG = error;
}

void Shutdown_UARTS(void)
{
    MXC_UART_Shutdown(READING_UART);
    MXC_UART_Shutdown(WRITING_UART);
    MXC_Delay(MXC_DELAY_SEC(1));
}

/******************************************************************************/
int main(void)
{
    int error, i, fail = 0;
    uint8_t TxData[BUFF_SIZE];
    uint8_t RxData[BUFF_SIZE];
    mxc_uart_regs_t *ConsoleUART = MXC_UART_GET_UART(CONSOLE_UART);

    printf("\n\n**************** UART Example ******************\n");
    printf("This example shows a loopback test between the 2 UARTs on the MAX32662.\n");
    printf("\nConnect UART0B TX to UART1A RX (P0.8 -> P0.3) for this example.\n");
    printf("The LEDs are used to indicate the success of the test.\nBlinking->Success, "
           "Solid->Failure\n");

    printf("\n-->UART Baud \t: %d Hz\n", UART_BAUD);
    printf("\n-->Test Length \t: %d bytes\n\n", BUFF_SIZE);

    printf("-->Initializing UARTS\n\n");

    // Print everything out
    while (!(ConsoleUART->status & MXC_F_UART_STATUS_TX_EM)) {}

    // Initialize the data buffers
    for (i = 0; i < BUFF_SIZE; i++) {
        TxData[i] = i;
    }
    memset(RxData, 0x0, BUFF_SIZE);

#ifdef DMA
    MXC_DMA_ReleaseChannel(0);
    MXC_NVIC_SetVector(DMA0_IRQn, DMA_Handler);
    NVIC_EnableIRQ(DMA0_IRQn);
#else
    NVIC_ClearPendingIRQ(MXC_UART_GET_IRQ(READ_IDX));
    NVIC_DisableIRQ(MXC_UART_GET_IRQ(READ_IDX));
    MXC_NVIC_SetVector(MXC_UART_GET_IRQ(READ_IDX), UART_Handler);
    NVIC_EnableIRQ(MXC_UART_GET_IRQ(READ_IDX));
#endif

    Console_Shutdown();
    MXC_Delay(MXC_DELAY_SEC(1));

    // Initialize the UART
    if ((error = MXC_UART_Init(WRITING_UART, UART_BAUD, MXC_UART_APB_CLK, MAP_B)) != E_NO_ERROR) {
        Shutdown_UARTS();
        Console_Init();
        printf("-->Error initializing UART: %d\n", error);
        printf("-->Example Failed\n");
        return error;
    }

    if ((error = MXC_UART_Init(READING_UART, UART_BAUD, MXC_UART_APB_CLK, MAP_A)) != E_NO_ERROR) {
        Shutdown_UARTS();
        Console_Init();
        printf("-->Error initializing UART: %d\n", error);
        printf("-->Example Failed\n");
        return error;
    }

    mxc_uart_req_t read_req;
    read_req.uart = READING_UART;
    read_req.rxData = RxData;
    read_req.rxLen = BUFF_SIZE;
    read_req.txLen = 0;
    read_req.callback = readCallback;

    mxc_uart_req_t write_req;
    write_req.uart = WRITING_UART;
    write_req.txData = TxData;
    write_req.txLen = BUFF_SIZE;
    write_req.rxLen = 0;
    write_req.callback = NULL;

    READ_FLAG = 1;
    DMA_FLAG = 1;

#ifdef DMA
    error = MXC_UART_TransactionDMA(&read_req);
#else
    error = MXC_UART_TransactionAsync(&read_req);
#endif
    if (error != E_NO_ERROR) {
        Shutdown_UARTS();
        Console_Init();
        printf("-->Error starting async read: %d\n", error);
        printf("-->Example Failed\n");
        LED_On(0);
        return error;
    }

#ifdef DMA
    error = MXC_UART_TransactionDMA(&write_req);
#else
    error = MXC_UART_Transaction(&write_req);
#endif
    if (error != E_NO_ERROR) {
        Shutdown_UARTS();
        Console_Init();
        printf("-->Error starting sync write: %d\n", error);
        printf("-->Example Failed\n");
        LED_On(0);
        return error;
    }

#ifdef DMA
    while (DMA_FLAG) {}
#else
    while (READ_FLAG) {}
    if (READ_FLAG != E_NO_ERROR) {
        fail++;
    }
#endif

    Shutdown_UARTS();
    Console_Init();

    if ((error = memcmp(RxData, TxData, BUFF_SIZE)) != 0) {
        printf("-->Error verifying Data: %d\n", error);
        fail++;
    } else {
        printf("-->Data verified\n");
    }

    if (fail != 0) {
        LED_On(0); // indicates FAIL
        printf("\n-->Example Failed\n");
        return E_FAIL;
    }

    printf("\n-->Example Succeeded\n");
    return E_NO_ERROR;
}