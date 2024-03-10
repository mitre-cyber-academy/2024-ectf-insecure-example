/* time_helper.c
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

/* common Espressif time_helper v5.6.3.002 */
#include "sdkconfig.h"
#include "time_helper.h"

#include <esp_log.h>
#include <esp_idf_version.h>

#if defined(ESP_IDF_VERSION_MAJOR) && defined(ESP_IDF_VERSION_MINOR)
    #if (ESP_IDF_VERSION_MAJOR == 5) && (ESP_IDF_VERSION_MINOR >= 1)
        #define HAS_ESP_NETIF_SNTP 1
        #include <lwip/apps/sntp.h>
        #include <esp_netif_sntp.h>
    #else
        #include <string.h>
        #include <esp_sntp.h>
    #endif
#else
    /* TODO Consider pre IDF v5? */
#endif

/* ESP-IDF uses a 64-bit signed integer to represent time_t starting from release v5.0
 * See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#year-2036-and-2038-overflow-issues
 */
const static char* TAG = "time_helper";

/* see https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html */
#ifndef TIME_ZONE
/*
 * PST represents Pacific Standard Time.
 * +8 specifies the offset from UTC (Coordinated Universal Time), indicating
 *   that Pacific Time is UTC-8 during standard time.
 * PDT represents Pacific Daylight Time.
 * M3.2.0 indicates that Daylight Saving Time (DST) starts on the
 *   second (2) Sunday (0) of March (3).
 * M11.1.0 indicates that DST ends on the first (1) Sunday (0) of November (11)
 */
    #define TIME_ZONE "PST+8PDT,M3.2.0,M11.1.0"
#endif /* not defined: TIME_ZONE, so we are setting our own */

#define NTP_RETRY_COUNT 10

/* NELEMS(x) number of elements
 * To determine the number of elements in the array, we can divide the total
 * size of the array by the size of the array element.
 * See https://stackoverflow.com/questions/37538/how-do-i-determine-the-size-of-my-array-in-c
 **/
#define NELEMS(x)  ( (int)(sizeof(x) / sizeof((x)[0])) )

/* See also CONFIG_LWIP_SNTP_MAX_SERVERS in sdkconfig */
#define NTP_SERVER_LIST ( (char*[]) {                        \
                                     "pool.ntp.org",         \
                                     "time.nist.gov",        \
                                     "utcnist.colorado.edu"  \
                                     }                       \
                        )
/* #define NTP_SERVER_COUNT using NELEMS:
 *
 *  (int)(sizeof(NTP_SERVER_LIST) / sizeof(NTP_SERVER_LIST[0]))
 */
#define NTP_SERVER_COUNT NELEMS(NTP_SERVER_LIST)

#ifndef CONFIG_LWIP_SNTP_MAX_SERVERS
    /* We should find max value in sdkconfig, if not set it to our count:*/
    #define CONFIG_LWIP_SNTP_MAX_SERVERS NTP_SERVER_COUNT
#endif

char* ntpServerList[NTP_SERVER_COUNT] = NTP_SERVER_LIST;

/* our NTP server list is global info */
extern char* ntpServerList[NTP_SERVER_COUNT];

/* Show the current date and time */
int esp_show_current_datetime()
{
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    setenv("TZ", TIME_ZONE, 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
    return 0;
}

/* the worst-case scenario is a hard-coded date/time */
int set_fixed_default_time(void)
{
    /* ideally, we'd like to set time from network,
     * but let's set a default time, just in case */
    struct tm timeinfo = {
        .tm_year = 2023 - 1900,
        .tm_mon  = 10,
        .tm_mday = 02,
        .tm_hour = 13,
        .tm_min  = 01,
        .tm_sec  = 05
    };
    struct timeval now;
    time_t interim_time;
    int ret = -1;

    /* set interim static time */
    interim_time = mktime(&timeinfo);

    ESP_LOGI(TAG, "Adjusting time from fixed value");
    now = (struct timeval){ .tv_sec = interim_time };
    ret = settimeofday(&now, NULL);

    return ret;
}

/* set_time_from_string(s)
 *
 * returns 0 = success if able to set the time from the provided string
 * error for any other value, typically -1 */
int set_time_from_string(char* time_buffer)
{
    /* expecting github default formatting: 'Thu Aug 31 12:41:45 2023 -0700' */
    const char *format = "%3s %3s %d %d:%d:%d %d %s";
    struct tm this_timeinfo;
    struct timeval now;
    time_t interim_time;
    char offset[6]; /* expecting trailing single quote, not used */
    char day_str[4];
    char month_str[4];
    int day, year, hour, minute, second;
    int quote_offset = 0;
    int ret = 0;

    /* we are expecting the string to be encapsulated in single quotes */
    if (*time_buffer == 0x27) {
        quote_offset = 1;
    }

    ret = sscanf(time_buffer + quote_offset,
                format,
                day_str, month_str,
                &day, &hour, &minute, &second, &year, &offset);

    if (ret == 8) {
        /* we found a match for all componets */

        const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

        for (int i = 0; i < 12; i++) {
            if (strcmp(month_str, months[i]) == 0) {
                this_timeinfo.tm_mon = i;
                break;
            }
        }

        this_timeinfo.tm_mday = day;
        this_timeinfo.tm_hour = hour;
        this_timeinfo.tm_min = minute;
        this_timeinfo.tm_sec = second;
        this_timeinfo.tm_year = year - 1900; /* Number of years since 1900 */

        interim_time = mktime(&this_timeinfo);
        now = (struct timeval){ .tv_sec = interim_time };
        ret = settimeofday(&now, NULL);
        ESP_LOGI(TAG, "Time updated to %s", time_buffer);
    }
    else {
        ESP_LOGE(TAG, "Failed to convert \"%s\" to a tm date.", time_buffer);
        ESP_LOGI(TAG, "Trying fixed date that was hard-coded.");
        set_fixed_default_time();
        ret = -1;
    }
    return ret;
}

/* set time; returns 0 if succecssfully configured with NTP */
int set_time(void)
{
#ifndef NTP_SERVER_COUNT
    ESP_LOGW(TAG, "Warning: no sntp server names defined. "
                  "Setting to empty list");
    #define NTP_SERVER_COUNT 0
    #warning "NTP not properly configured"
#endif /* not defined: NTP_SERVER_COUNT */

#ifdef HAS_ESP_NETIF_SNTP
    #if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
                                       NTP_SERVER_COUNT,
                                       ESP_SNTP_SERVER_LIST(ntpServerList[0])
                                   );
    #else
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(ntpServerList[0]);
    #endif /* CONFIG_LWIP_SNTP_MAX_SERVERS > 1 */
#endif /* HAS_ESP_NETIF_SNTP */

    int ret = 0;
    int i = 0; /* counter for time servers */

    ESP_LOGI(TAG, "Setting the time. Startup time:");
    esp_show_current_datetime();

#ifdef LIBWOLFSSL_VERSION_GIT_HASH_DATE
    /* initialy set a default approximate time from recent git commit */
    ESP_LOGI(TAG, "Found git hash date, attempting to set system date.");
    set_time_from_string(LIBWOLFSSL_VERSION_GIT_HASH_DATE);
    esp_show_current_datetime();

    ret = -4;
#else
    /* otherwise set a fixed time that was hard coded */
    set_fixed_default_time();
    ret = -3;
#endif

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    config.smooth_sync = true;
#endif

    if (NTP_SERVER_COUNT) {
        /* next, let's setup NTP time servers
         *
         * see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#sntp-time-synchronization
         *
         * WARNING: do not set operating mode while SNTP client is running!
         */
        /* TODO Consider esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);  */
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        if (NTP_SERVER_COUNT > CONFIG_LWIP_SNTP_MAX_SERVERS) {
            ESP_LOGW(TAG, "WARNING: %d NTP Servers defined, but "
                          "CONFIG_LWIP_SNTP_MAX_SERVERS = %d",
                           NTP_SERVER_COUNT,CONFIG_LWIP_SNTP_MAX_SERVERS);
        }
        ESP_LOGI(TAG, "sntp_setservername:");
        for (i = 0; i < CONFIG_LWIP_SNTP_MAX_SERVERS; i++) {
            const char* thisServer = ntpServerList[i];
            if (strncmp(thisServer, "\x00", 1) == 0) {
                /* just in case we run out of NTP servers */
                break;
            }
            ESP_LOGI(TAG, "%s", thisServer);
            sntp_setservername(i, thisServer);
        }
    #ifdef HAS_ESP_NETIF_SNTP
        ret = esp_netif_sntp_init(&config);
    #else
        ESP_LOGW(TAG,"Warning: Consider upgrading ESP-IDF to take advantage "
                     "of updated SNTP libraries");
    #endif
        if (ret == ESP_OK) {
            ESP_LOGV(TAG, "Successfully called esp_netif_sntp_init");
        }
        else {
            ESP_LOGE(TAG, "ERROR: esp_netif_sntp_init return = %d", ret);
        }

        sntp_init();
        switch (ret) {
            case ESP_ERR_INVALID_STATE:
                break;
            default:
                break;
        }
        ESP_LOGI(TAG, "sntp_init done.");
    }
    else {
        ESP_LOGW(TAG, "No sntp time servers found.");
        ret = -1;
    }
    return ret;
}

/* wait for NTP to actually set the time */
int set_time_wait_for_ntp(void)
{
    int ret = 0;
#ifdef HAS_ESP_NETIF_SNTP
    int ntp_retry = 0;
    const int ntp_retry_count = NTP_RETRY_COUNT;

    ret = esp_netif_sntp_start();

    ret = esp_netif_sntp_sync_wait(500 / portTICK_PERIOD_MS);
#endif /* HAS_ESP_NETIF_SNTP */
    esp_show_current_datetime();

#ifdef HAS_ESP_NETIF_SNTP
    while (ret == ESP_ERR_TIMEOUT && (ntp_retry++ < ntp_retry_count)) {
        ret = esp_netif_sntp_sync_wait(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Waiting for NTP to sync time... (%d/%d)",
                       ntp_retry,
                       ntp_retry_count);
        esp_show_current_datetime();
    }
#endif /* HAS_ESP_NETIF_SNTP */

#ifdef TIME_ZONE
    setenv("TZ", TIME_ZONE, 1);
    tzset();
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Successfuly set time via NTP servers.");
        }
    else {
        ESP_LOGW(TAG, "Warning: Failed to set time with NTP: "
                      "result = 0x%0x: %s",
                       ret, esp_err_to_name(ret));
    }
    return ret;
}
