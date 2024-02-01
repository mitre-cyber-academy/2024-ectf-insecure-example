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
 * @file        main.c
 * @brief       Configures and starts the RTC and demonstrates the use of the alarms.
 * @details     The RTC is enabled and the sub-second alarm set to trigger every 250 ms.
 *              P2.25 (LED0) is toggled each time the sub-second alarm triggers.  The
 *              time-of-day alarm is set to 2 seconds.  When the time-of-day alarm
 *              triggers, the rate of the sub-second alarm is switched to 500 ms.  The
 *              time-of-day alarm is then rearmed for another 2 sec.  Pressing SW2 will
 *              output the current value of the RTC to the console UART.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "nvic_table.h"
#include "board.h"
#include "rtc.h"
#include "led.h"
#include "pb.h"
#include "mxc_delay.h"

/***** Definitions *****/
#define LED_ALARM 0
#define LED_TODA 1

#define TIME_OF_DAY_SEC 10
#define SUBSECOND_MSEC_0 250
#define SUBSECOND_MSEC_1 500

#define MSEC_TO_RSSA(x) \
    (0 - ((x * 4096) /  \
          1000)) /* Converts a time in milleseconds to the equivalent RSSA register value. */

#define SECS_PER_MIN 60
#define SECS_PER_HR (60 * SECS_PER_MIN)
#define SECS_PER_DAY (24 * SECS_PER_HR)

/***** Globals *****/
int ss_interval = SUBSECOND_MSEC_0;

/***** Functions *****/
void RTC_IRQHandler(void)
{
    uint32_t time;
    int flags = MXC_RTC_GetFlags();

    /* Check sub-second alarm flag. */
    if (flags & MXC_RTC_INT_FL_SHORT) {
        LED_Toggle(LED_ALARM);
        MXC_RTC_ClearFlags(MXC_RTC_INT_FL_SHORT);
    }

    /* Check time-of-day alarm flag. */
    if (flags & MXC_RTC_INT_FL_LONG) {
        MXC_RTC_ClearFlags(MXC_RTC_INT_FL_LONG);
        LED_Toggle(LED_TODA);

        while (MXC_RTC_DisableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {}

        /* Set a new alarm TIME_OF_DAY_SEC seconds from current time. */
        /* Don't need to check busy here as it was checked in MXC_RTC_DisableInt() */
        MXC_RTC_GetSeconds(&time);

        if (MXC_RTC_SetTimeofdayAlarm(time + TIME_OF_DAY_SEC) != E_NO_ERROR) {
            /* Handle Error */
        }

        while (MXC_RTC_EnableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {}

        // Toggle the sub-second alarm interval.
        if (ss_interval == SUBSECOND_MSEC_0) {
            ss_interval = SUBSECOND_MSEC_1;
        } else {
            ss_interval = SUBSECOND_MSEC_0;
        }

        while (MXC_RTC_DisableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {}

        if (MXC_RTC_SetSubsecondAlarm(MSEC_TO_RSSA(ss_interval)) != E_NO_ERROR) {
            /* Handle Error */
        }

        while (MXC_RTC_EnableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {}
    }

    return;
}

volatile int buttonPressed = 0;
void buttonHandler()
{
    buttonPressed = 1;
}

void printTime()
{
    int day, hr, min, err;
    uint32_t sec, rtc_readout;
    double subsec;

    do {
        err = MXC_RTC_GetSubSeconds(&rtc_readout);
    } while (err != E_NO_ERROR);
    subsec = rtc_readout / 4096.0;

    do {
        err = MXC_RTC_GetSeconds(&rtc_readout);
    } while (err != E_NO_ERROR);
    sec = rtc_readout;

    day = sec / SECS_PER_DAY;
    sec -= day * SECS_PER_DAY;

    hr = sec / SECS_PER_HR;
    sec -= hr * SECS_PER_HR;

    min = sec / SECS_PER_MIN;
    sec -= min * SECS_PER_MIN;

    subsec += sec;

    printf("\nCurrent Time (dd:hh:mm:ss): %02d:%02d:%02d:%05.2f\n", day, hr, min, subsec);
}

// *****************************************************************************
int main(void)
{
    printf("\n*************************** RTC Example ****************************\n\n");
    printf("The RTC is enabled and the sub-second alarm set to trigger every %d ms.\n",
           SUBSECOND_MSEC_0);
    printf("(LED0) is toggled each time the sub-second alarm triggers.\n\n");
    printf("The time-of-day alarm is set to %d seconds.  When the time-of-day alarm\n",
           TIME_OF_DAY_SEC);
    printf("triggers, the rate of the sub-second alarm is switched to %d ms.\n\n",
           SUBSECOND_MSEC_1);
    printf("(LED1) is toggled each time the time-of-day alarm triggers.\n\n");
    printf("The time-of-day alarm is then rearmed for another %d sec.  Pressing SW1\n",
           TIME_OF_DAY_SEC);
    printf("will output the current value of the RTC to the console UART.\n\n");

    NVIC_EnableIRQ(RTC_IRQn);

    /* Setup callback to receive notification of when button is pressed. */
    PB_RegisterCallback(0, (pb_callback)buttonHandler);

    /* Turn LED off initially */
    LED_On(LED_ALARM);
    LED_On(LED_TODA);

    if (MXC_RTC_Init(0, 0) != E_NO_ERROR) {
        printf("Failed RTC Initialization\n");
        printf("Example Failed\n");

        while (1) {}
    }

    if (MXC_RTC_DisableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_SetTimeofdayAlarm(TIME_OF_DAY_SEC) != E_NO_ERROR) {
        printf("Failed RTC_SetTimeofdayAlarm\n");
        printf("Example Failed\n");

        while (1) {}
    }

    if (MXC_RTC_EnableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_DisableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_SetSubsecondAlarm(MSEC_TO_RSSA(SUBSECOND_MSEC_0)) != E_NO_ERROR) {
        printf("Failed RTC_SetSubsecondAlarm\n");
        printf("Example Failed\n");

        while (1) {}
    }

    if (MXC_RTC_EnableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_SquareWaveStart(MXC_RTC_F_512HZ) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_Start() != E_NO_ERROR) {
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");

        while (1) {}
    }

    printf("RTC started\n");
    printTime();

    while (1) {
        if (buttonPressed) {
            /* Show the time elapsed. */
            printTime();
            /* Delay for switch debouncing. */
            MXC_Delay(MXC_DELAY_MSEC(100));
            /* Re-arm switch detection. */
            buttonPressed = 0;
        }
    }
}
