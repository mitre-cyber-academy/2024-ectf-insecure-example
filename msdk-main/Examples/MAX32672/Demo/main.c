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
 * @brief   This example demonstrates the use of the TFT Display, UART Terminal, RTC, Pushbuttons, and LEDs.
 * @details This example displays the uptime to the TFT Display and Terminal,
 *          toggles the LED at 1-2Hz depending on pushbutton press, and shows
 *          the chip information if the pushbutton is pressed 5 times.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_delay.h"
#include "mxc_device.h"
#include "mxc_sys.h"
#include "pb.h"
#include "led.h"
#include "board.h"
#include "rtc.h"
#include "bitmap.h"
#include "tft_st7735.h"

/***** Definitions *****/

#define SECS_PER_MIN 60
#define SECS_PER_HR (60 * SECS_PER_MIN)

/***** Globals *****/

int buttonPressedCount = 0;

/***** Functions *****/

volatile int buttonPressed = 0;
void buttonHandler(void *pb)
{
    buttonPressed = 1;
}

// This function runs at every 250ms to account for debouncing.
void checkForButtonRelease(void)
{
    if (buttonPressed && !PB_Get(0)) {
        buttonPressedCount++;
        buttonPressed = 0;
    }
}

// *****************************************************************************
int main(void)
{
    int i;
    int pb_state = 0;
    uint8_t usn[MXC_SYS_USN_LEN];
    area_t usn_printf_area;
    area_t units_printf_area;
    area_t uptime_printf_area;
    int hr, min;
    uint32_t sec;
    int error;

    printf("**** MAX32672 EV Kit Demo ****\n");

    // Predefine Printf Area for displaying USN on TFT.
    usn_printf_area.x = 2;
    usn_printf_area.y = 3;
    usn_printf_area.w = 128;
    usn_printf_area.h = 128;

    MXC_SYS_GetUSN(usn, NULL);

    PB_RegisterCallback(0, (pb_callback)buttonHandler);

    MXC_TFT_Init();
    MXC_TFT_ShowImage(2, 3, (int)&logo_rgb565[0]);

    MXC_Delay(MXC_DELAY_SEC(2));

    MXC_TFT_SetBackGroundColor(WHITE);

    MXC_TFT_SetFont((int)&Liberation_Sans16x16[0]);

    error = MXC_RTC_Init(0, 0);
    if (error != E_NO_ERROR) {
        printf("Failed RTC Initialization\n");
        return error;
    }

    error = MXC_RTC_Start();
    if (error != E_NO_ERROR) {
        printf("Failed RTC_Start\n");
        return error;
    }

    // Set print area
    units_printf_area.x = 24;
    units_printf_area.y = 25;
    units_printf_area.w = 128;
    units_printf_area.h = 30;
    MXC_TFT_ConfigPrintf(&units_printf_area);
    MXC_TFT_Printf("hh:mm:ss");

    uptime_printf_area.x = 22;
    uptime_printf_area.y = 58;
    uptime_printf_area.w = 128;
    uptime_printf_area.h = 30;
    MXC_TFT_ConfigPrintf(&uptime_printf_area);

    while (1) {
        // This entire routine until first LED Toggles takes around ~180ms.
        MXC_RTC_GetSeconds(&sec);

        hr = sec / SECS_PER_HR;
        sec -= hr * SECS_PER_HR;

        min = sec / SECS_PER_MIN;
        sec -= min * SECS_PER_MIN;

        printf("\n(hhh:mm:ss): %03d:%02d:%02d\n\n", hr, min, sec);
        MXC_TFT_Printf("%03d:%02d:%02d\n", hr, min, sec);

        if (buttonPressedCount > 4) {
            buttonPressedCount = 0;

            // Printf USN on bottom of screen.
            MXC_TFT_SetBackGroundColor(WHITE);

            MXC_TFT_ConfigPrintf(&usn_printf_area);

            // Print revision and USN.
            MXC_TFT_Printf("Rev: %x\n", MXC_GCR->revision);
            MXC_TFT_Printf(
                "USN:\n  %02x%02x%02x%02x%02x\n   -%02x%02x%02x%02x\n   -%02x%02x%02x%02x\n",
                usn[0], usn[1], usn[2], usn[3], usn[4], usn[5], usn[6], usn[7], usn[8], usn[9],
                usn[10], usn[11], usn[12]);

            printf("\nRev: %x\n", MXC_GCR->revision);
            printf("USN: ");
            for (i = 0; i < MXC_SYS_USN_LEN; i++) {
                printf("%02x", usn[i]);
            }
            printf("\n\n");

            MXC_TFT_Printf("\nContinue in\n");
            for (i = 5; i > 0; i--) {
                MXC_TFT_Printf("%d..", i);
                MXC_Delay(MXC_DELAY_SEC(1));
            }

            // Clear screen and continue uptime.
            MXC_TFT_SetBackGroundColor(WHITE);

            MXC_TFT_ConfigPrintf(&units_printf_area);
            MXC_TFT_Printf("hh:mm:ss");

            MXC_TFT_ConfigPrintf(&uptime_printf_area);
            MXC_TFT_Printf("%03d:%02d:%02d\n", hr, min, sec);
        }

        pb_state = PB_Get(0);

        LED_On(0);
        checkForButtonRelease();

        // Invert LED1 if PB0 is pressed.
        if (pb_state) {
            LED_Off(1);
        } else {
            LED_On(1);
        }

        // Delay for 500ms
        MXC_Delay(250000);

        // Debouncing purposes
        checkForButtonRelease();
        MXC_Delay(250000);

        LED_Off(0);
        checkForButtonRelease();

        // Invert LED1 if PB0 is pressed.
        if (pb_state) {
            LED_On(1);
        } else {
            LED_Off(1);
        }

        // Delay for 500ms
        MXC_Delay(250000);
        // Debouncing purposes for displaying chip info.
        checkForButtonRelease();

        // Set as 70000 (70ms) instead of 250000 (250ms) because logic at beginning of while
        //  loop takes around ~180ms.
        MXC_Delay(70000);
    }
}