/* main.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */
#include "sdkconfig.h"
#include "main.h"

/* ESP specific */
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_event.h>

/* wolfSSL */
#include <wolfssl/wolfcrypt/settings.h>
#include <user_settings.h>
#include <wolfssl/wolfcrypt/port/Espressif/esp32-crypt.h>
#ifndef WOLFSSL_ESPIDF
    #warning "Problem with wolfSSL user_settings."
    #warning "Check components/wolfssl/include"
#endif

/* this project */
#include "client-tls.h"
#include "time_helper.h"

#ifndef CONFIG_IDF_TARGET_ESP32H2
    /* There's no WiFi on ESP32-H2.
     * For wired ethernet, see:
     * https://github.com/wolfSSL/wolfssl-examples/tree/master/ESP32/TLS13-ENC28J60-client */
    #include "wifi_connect.h"
#endif

#ifdef WOLFSSL_TRACK_MEMORY
    #include <wolfssl/wolfcrypt/mem_track.h>
#endif

static const char* const TAG = "TLS Client";

#if defined(WOLFSSL_ESPWROOM32SE) && defined(HAVE_PK_CALLBACKS) \
                                  && defined(WOLFSSL_ATECC508A)

#include "wolfssl/wolfcrypt/port/atmel/atmel.h"

/* when you want to use a custom slot allocation */
/* enable the definition CUSTOM_SLOT_ALLOCATION. */

#if defined(CUSTOM_SLOT_ALLOCATION)

static byte mSlotList[ATECC_MAX_SLOT];

int atmel_set_slot_allocator(atmel_slot_alloc_cb alloc, atmel_slot_dealloc_cb dealloc);

/* initialize slot array */
void my_atmel_slotInit()
{
    int i;
    for(i = 0;i < ATECC_MAX_SLOT;i++) {
        mSlotList[i] = ATECC_INVALID_SLOT;
    }
}

/* allocate slot depending on slotType */
int my_atmel_alloc(int slotType)
{
    int i, slot = -1;

    switch(slotType){
        case ATMEL_SLOT_ENCKEY:
            slot = 4;
            break;
        case ATMEL_SLOT_DEVICE:
            slot = 0;
            break;
        case ATMEL_SLOT_ECDHE:
            slot = 0;
            break;
        case ATMEL_SLOT_ECDHE_ENC:
            slot = 4;
            break;
        case ATMEL_SLOT_ANY:
            for(i = 0;i < ATECC_MAX_SLOT;i++){
                if(mSlotList[i] == ATECC_INVALID_SLOT){
                    slot = i;
                    break;
                }
            }
    }

    return slot;
}

/* free slot array       */
void my_atmel_free(int slotId)
{
    if(slotId >= 0 && slotId < ATECC_MAX_SLOT){
        mSlotList[slotId] = ATECC_INVALID_SLOT;
    }
}
#endif /* CUSTOM_SLOT_ALLOCATION                                       */
#endif /* WOLFSSL_ESPWROOM32SE && HAVE_PK_CALLBACK && WOLFSSL_ATECC508A */

/* for FreeRTOS */
void app_main(void)
{
    int stack_start = 0;
    esp_err_t ret = 0;
    ESP_LOGI(TAG, "---------------- wolfSSL TLS Client Example ------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "---------------------- BEGIN MAIN ----------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
#ifdef ESP_TASK_MAIN_STACK
    ESP_LOGI(TAG, "ESP_TASK_MAIN_STACK: %d", ESP_TASK_MAIN_STACK);
#endif
#ifdef TASK_EXTRA_STACK_SIZE
    ESP_LOGI(TAG, "TASK_EXTRA_STACK_SIZE: %d", TASK_EXTRA_STACK_SIZE);
#endif
#ifdef INCLUDE_uxTaskGetStackHighWaterMark
    ESP_LOGI(TAG, "CONFIG_ESP_MAIN_TASK_STACK_SIZE = %d bytes (%d words)",
                   CONFIG_ESP_MAIN_TASK_STACK_SIZE,
                   (int)(CONFIG_ESP_MAIN_TASK_STACK_SIZE / sizeof(void*)));

    /* Returns the high water mark of the stack associated with xTask. That is,
     * the minimum free stack space there has been (in bytes not words, unlike
     * vanilla FreeRTOS) since the task started. The smaller the returned
     * number the closer the task has come to overflowing its stack.
     * see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html
     */
    stack_start = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "Stack Start HWM: %d bytes", stack_start);
#endif

#ifdef HAVE_VERSION_EXTENDED_INFO
    esp_ShowExtendedSystemInfo();
#endif

    /* Set time for cert validation.
     * Some lwIP APIs, including SNTP functions, are not thread safe. */
    ret = set_time(); /* need to setup NTP before WiFi */

    /* Optionally erase flash */
    /* ESP_ERROR_CHECK(nvs_flash_erase()); */

#ifdef FOUND_PROTOCOL_EXAMPLES_DIR
    ESP_LOGI(TAG, "FOUND_PROTOCOL_EXAMPLES_DIR active, using example code.");
    ESP_ERROR_CHECK(nvs_flash_init());

    #if defined(CONFIG_IDF_TARGET_ESP32H2)
        ESP_LOGE(TAG, "There's no WiFi on ESP32-H2.");
    #else
        #ifdef CONFIG_EXAMPLE_WIFI_SSID
            if (XSTRCMP(CONFIG_EXAMPLE_WIFI_SSID, "myssid") == 0) {
                ESP_LOGW(TAG, "WARNING: CONFIG_EXAMPLE_WIFI_SSID is myssid.");
                ESP_LOGW(TAG, "  Do you have a WiFi AP called myssid, or ");
                ESP_LOGW(TAG, "  did you forget the ESP-IDF configuration?");
            }
        #else
            #define CONFIG_EXAMPLE_WIFI_SSID "myssid"
            ESP_LOGW(TAG, "WARNING: CONFIG_EXAMPLE_WIFI_SSID not defined.");
        #endif
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        ESP_ERROR_CHECK(example_connect());
    #endif
#else
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    #if defined(CONFIG_IDF_TARGET_ESP32H2)
        ESP_LOGE(TAG, "There's no WiFi on ESP32-H2. ");
    #else
        /* Initialize WiFi */
        ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
        ret = wifi_init_sta();
        while (ret != 0) {
            ESP_LOGI(TAG, "Waiting...");
            vTaskDelay(60000 / portTICK_PERIOD_MS);
            ESP_LOGI(TAG, "Trying WiFi again...");
            ret = wifi_init_sta();
        }
    #endif
#endif

    /* Once we are connected to the network, start & wait for NTP time */
    ret = set_time_wait_for_ntp();

    if (ret < -1) {
        /* a value of -1 means there was no NTP server, so no need to wait */
        ESP_LOGI(TAG, "Waiting 10 more seconds for NTP to complete." );
        vTaskDelay(10000 / portTICK_PERIOD_MS); /* brute-force solution */
        esp_show_current_datetime();
    }

    /* HWM is maximum amount of stack space that has been unused, in bytes
     * not words (unlike vanilla freeRTOS). */
    ESP_LOGI(TAG, "Initial Stack Used (before wolfSSL Server): %d bytes",
                   CONFIG_ESP_MAIN_TASK_STACK_SIZE
                   - (uxTaskGetStackHighWaterMark(NULL))
            );
    ESP_LOGI(TAG, "Starting TLS Client task ...\n");

#if defined(SINGLE_THREADED)
    /* just call the task */
    tls_smp_client_task((void*)NULL);
#else
    tls_args args[1] = {0};
    /* start a thread with the task */
    args[0].loops = 10;
    args[0].port = 11111;
    tls_smp_client_init(args);
/* optional additional client threads
    tls_smp_client_init(args);
    tls_smp_client_init(args);
    tls_smp_client_init(args);
    tls_smp_client_init(args);
    tls_smp_client_init(args);
    tls_smp_client_init(args);
    tls_smp_client_init(args);
*/
#endif

    ESP_LOGV(TAG, "\n\nvTaskDelete...\n\n");
    vTaskDelete(NULL);
    /* done */
    while (1) {
        ESP_LOGV(TAG, "\n\nLoop...\n\n");
#ifdef INCLUDE_uxTaskGetStackHighWaterMark
        ESP_LOGI(TAG, "Stack HWM: %d", uxTaskGetStackHighWaterMark(NULL));

        ESP_LOGI(TAG, "Stack used: %d", CONFIG_ESP_MAIN_TASK_STACK_SIZE
                                        - (uxTaskGetStackHighWaterMark(NULL) ));
#endif

#if defined(SINGLE_THREADED)
        ESP_LOGV(TAG, "\n\nDone!\n\n");
        while (1);
#else
        vTaskDelay(60000);
#endif
    } /* done whle */

} /* app_main */
