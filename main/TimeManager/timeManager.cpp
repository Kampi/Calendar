/*
 * timeManager.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Time manager implementation with RTC and SNTP synchronization.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Errors and commissions should be reported to DanielKampert@kampis-elektroecke.de
 */

#include <esp_log.h>
#include <esp_sntp.h>
#include <esp_timer.h>

#include <nvs_flash.h>
#include <nvs.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <sys/time.h>
#include <string.h>

#include "timeManager.h"
#include "../SNTP/sntp.h"
#include "../Devices/RTC/bm8563.h"

#define TIMEMANAGER_NVS_NAMESPACE   "time_mgr"
#define TIMEMANAGER_NVS_LAST_SYNC   "last_sync"

ESP_EVENT_DEFINE_BASE(TIMEMANAGER_EVENTS);

static const char *TAG = "TimeManager";

typedef struct {
    bool isInitialized;
    bool isNTPInitialized;
    bool isTimeValid;
    bool isNTPSynced;
    time_t last_sync_time;
    uint32_t sync_interval_days;
    nvs_handle_t NVS_Handle;
    i2c_master_dev_handle_t RTC_Handle;
    char Timezone[64];
    char NTP_Server[128];
} TimeManager_State_t;

static TimeManager_State_t _TimeManager_State = {
    .isInitialized = false,
    .isNTPInitialized = false,
    .isTimeValid = false,
    .isNTPSynced = false,
    .last_sync_time = 0,
    .sync_interval_days = 2,
    .NVS_Handle = 0,
    .RTC_Handle = NULL,
    .Timezone = "",
    .NTP_Server = "",
};

/** @brief Save last SNTP sync timestamp to NVS
 */
static esp_err_t save_last_sync_time(time_t sync_time)
{
    esp_err_t Error = nvs_set_i64(_TimeManager_State.NVS_Handle, TIMEMANAGER_NVS_LAST_SYNC, (int64_t)sync_time);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save last sync time to NVS: %d!", Error);

        return Error;
    }

    Error = nvs_commit(_TimeManager_State.NVS_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %d!", Error);
    }

    return Error;
}

static void _TimeManager_on_Event(void *p_HandlerArgs, esp_event_base_t Base, int32_t ID, void *p_Data)
{
    if (ID == SNTP_EVENT_SNTP_SYNCED) {
        struct timeval *p_TimeVal = (struct timeval *)p_Data;
        struct tm TimeInfo;

        localtime_r(&p_TimeVal->tv_sec, &TimeInfo);

        /* Set system time and sync to RTC */
        TimeManager_SetTime(&TimeInfo, true);

        _TimeManager_State.isNTPSynced = true;
        _TimeManager_State.last_sync_time = p_TimeVal->tv_sec;

        /* Save last sync time to NVS */
        save_last_sync_time(_TimeManager_State.last_sync_time);

        ESP_LOGI(TAG, "Time synced via SNTP: %04d-%02d-%02d %02d:%02d:%02d",
                 TimeInfo.tm_year + 1900, TimeInfo.tm_mon + 1, TimeInfo.tm_mday,
                 TimeInfo.tm_hour, TimeInfo.tm_min, TimeInfo.tm_sec);
    }
}

esp_err_t TimeManager_Init(const TimeManager_Config_t *p_Config)
{
    esp_err_t Error;
    int64_t last_sync = 0;

    if ((p_Config == NULL) || (p_Config->NTPServer[0] == '\0')) {
        ESP_LOGE(TAG, "Invalid configuration!");

        return ESP_ERR_INVALID_ARG;
    } else if (_TimeManager_State.isInitialized) {
        ESP_LOGW(TAG, "Time manager already initialized");

        return ESP_OK;
    }

    Error = nvs_open(TIMEMANAGER_NVS_NAMESPACE, NVS_READWRITE, &_TimeManager_State.NVS_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %d!", Error);

        return Error;
    }

    /* Load last SNTP sync time from NVS */
    Error = nvs_get_i64(_TimeManager_State.NVS_Handle, TIMEMANAGER_NVS_LAST_SYNC, &last_sync);
    if (Error == ESP_OK) {
        _TimeManager_State.last_sync_time = (time_t)last_sync;
        ESP_LOGI(TAG, "Last SNTP sync loaded from NVS: %ld", static_cast<long>(_TimeManager_State.last_sync_time));
    } else if (Error == ESP_ERR_NVS_NOT_FOUND) {
        _TimeManager_State.last_sync_time = 0;
        ESP_LOGI(TAG, "No previous SNTP sync found in NVS");
    } else {
        ESP_LOGW(TAG, "Failed to load last sync time from NVS: %d", Error);
        _TimeManager_State.last_sync_time = 0;
    }

    /* Store sync interval (convert from days to seconds internally) */
    _TimeManager_State.sync_interval_days = p_Config->SyncInterval;

    /* Initialize RTC */
    Error = BM8563_Init(p_Config->BusHandle, &_TimeManager_State.RTC_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RTC: %d!", Error);

        nvs_close(_TimeManager_State.NVS_Handle);

        return Error;
    }

    /* Store config for later SNTP initialization */
    strncpy(_TimeManager_State.Timezone, p_Config->Timezone, sizeof(_TimeManager_State.Timezone) - 1);
    strncpy(_TimeManager_State.NTP_Server, p_Config->NTPServer, sizeof(_TimeManager_State.NTP_Server) - 1);

    _TimeManager_State.isTimeValid = BM8563_IsValid(&_TimeManager_State.RTC_Handle);
    _TimeManager_State.isInitialized = true;

    ESP_LOGI(TAG, "Time manager initialized successfully (RTC time valid: %d)", _TimeManager_State.isTimeValid);

    return TimeManager_ClearAlarmFlag();
}

esp_err_t TimeManager_Deinit(void)
{
    if (_TimeManager_State.isInitialized == false) {
        return ESP_OK;
    }

    if (_TimeManager_State.isNTPInitialized) {
        SNTP_Deinit();
        _TimeManager_State.isNTPInitialized = false;
        _TimeManager_State.isNTPSynced = false;
    }

    BM8563_Deinit(&_TimeManager_State.RTC_Handle);

    if (_TimeManager_State.NVS_Handle != 0) {
        nvs_close(_TimeManager_State.NVS_Handle);
        _TimeManager_State.NVS_Handle = 0;
    }

    _TimeManager_State.isInitialized = false;
    _TimeManager_State.isTimeValid = false;
    _TimeManager_State.last_sync_time = 0;

    ESP_LOGD(TAG, "Time manager deinitialized");

    return ESP_OK;
}

esp_err_t TimeManager_InitSNTP(void)
{
    esp_err_t Error;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    } else if (_TimeManager_State.isNTPInitialized) {
        ESP_LOGW(TAG, "SNTP already initialized");

        return ESP_OK;
    } else if (_TimeManager_State.NTP_Server[0] == '\0') {
        ESP_LOGE(TAG, "No NTP server configured!");

        return ESP_ERR_INVALID_ARG;
    }

    /* Reset sync flag for fresh start */
    _TimeManager_State.isNTPSynced = false;

    Error = SNTP_Init(_TimeManager_State.Timezone, _TimeManager_State.NTP_Server);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP: %d!", Error);

        return Error;
    }

    esp_event_handler_register(SNTP_EVENTS, ESP_EVENT_ANY_ID, _TimeManager_on_Event, NULL);

    _TimeManager_State.isNTPInitialized = true;
    ESP_LOGI(TAG, "SNTP initialized successfully");

    return ESP_OK;
}

esp_err_t TimeManager_GetTime(struct tm *p_TimeInfo)
{
    time_t Now;

    if (p_TimeInfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    Now = time(NULL);
    localtime_r(&Now, p_TimeInfo);

    return ESP_OK;
}

esp_err_t TimeManager_SetTime(const struct tm *p_TimeInfo, bool SyncRTC)
{
    if (p_TimeInfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Convert tm to time_t */
    struct tm temp = *p_TimeInfo;
    time_t t = mktime(&temp);

    if (t == -1) {
        ESP_LOGE(TAG, "Invalid time!");

        return ESP_ERR_INVALID_ARG;
    }

    /* Set system time */
    struct timeval p_TV = {
        .tv_sec = t,
        .tv_usec = 0
    };

    if (settimeofday(&p_TV, NULL) != 0) {
        ESP_LOGE(TAG, "Failed to set system time");

        return ESP_FAIL;
    }

    _TimeManager_State.isTimeValid = true;
    _TimeManager_State.last_sync_time = t;

    ESP_LOGD(TAG, "System time set manually: %04d-%02d-%02d %02d:%02d:%02d",
             p_TimeInfo->tm_year + 1900, p_TimeInfo->tm_mon + 1, p_TimeInfo->tm_mday,
             p_TimeInfo->tm_hour, p_TimeInfo->tm_min, p_TimeInfo->tm_sec);

    /* Sync to RTC if requested */
    if (SyncRTC && _TimeManager_State.isInitialized) {
        return TimeManager_SyncToRTC();
    }

    return ESP_OK;
}

esp_err_t TimeManager_SyncFromRTC(void)
{
    esp_err_t Error;
    struct tm p_TimeInfo;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    }

    /* Check if RTC has valid time */
    if (BM8563_IsValid(&_TimeManager_State.RTC_Handle) == false) {
        ESP_LOGW(TAG, "RTC does not have valid time");

        return ESP_ERR_INVALID_STATE;
    }

    /* Read time from RTC */
    Error = BM8563_GetTime(&_TimeManager_State.RTC_Handle, &p_TimeInfo);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time from RTC!");

        return Error;
    }

    /* Set system time */
    Error = TimeManager_SetTime(&p_TimeInfo, false);
    if (Error == ESP_OK) {
        ESP_LOGD(TAG, "System time synced from RTC: %04d-%02d-%02d %02d:%02d:%02d",
                 p_TimeInfo.tm_year + 1900, p_TimeInfo.tm_mon + 1, p_TimeInfo.tm_mday,
                 p_TimeInfo.tm_hour, p_TimeInfo.tm_min, p_TimeInfo.tm_sec);
    }

    return Error;
}

esp_err_t TimeManager_SyncFromSNTP(uint32_t TimeoutMs)
{
    uint32_t Timeout = TimeoutMs;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    } else if (_TimeManager_State.isNTPInitialized == false) {
        ESP_LOGE(TAG, "SNTP not initialized - call TimeManager_InitSNTP() first!");

        return ESP_ERR_INVALID_STATE;
    } else if (_TimeManager_State.isNTPSynced) {
        ESP_LOGD(TAG, "SNTP already synced");

        return ESP_OK;
    }

    /* Wait for SNTP sync */
    do {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (_TimeManager_State.isNTPSynced) {
            return ESP_OK;
        }
    } while ((Timeout > 0) && (Timeout -= 100) > 0);

    ESP_LOGW(TAG, "SNTP sync timeout after %lu ms", static_cast<unsigned long>(TimeoutMs));

    return ESP_FAIL;
}

esp_err_t TimeManager_SyncToRTC(void)
{
    esp_err_t Error;
    struct tm p_TimeInfo;

    if (_TimeManager_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    } else if (_TimeManager_State.isTimeValid == false) {
        ESP_LOGW(TAG, "System time not valid, cannot sync to RTC");

        return ESP_ERR_INVALID_STATE;
    }

    /* Get current system time */
    Error = TimeManager_GetTime(&p_TimeInfo);
    if (Error != ESP_OK) {
        return Error;
    }

    /* Write to RTC */
    Error = BM8563_SetTime(&_TimeManager_State.RTC_Handle, &p_TimeInfo);
    if (Error == ESP_OK) {
        ESP_LOGD(TAG, "RTC synced with system time");
    }

    return Error;
}

bool TimeManager_IsTimeValid(void)
{
    return _TimeManager_State.isTimeValid;
}

esp_err_t TimeManager_SetAlarm(const struct tm *p_TimeInfo)
{
    esp_err_t Error;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    } else if (p_TimeInfo == NULL) {
        ESP_LOGE(TAG, "Invalid time info pointer!");

        return ESP_ERR_INVALID_ARG;
    }

    Error = BM8563_SetAlarm(&_TimeManager_State.RTC_Handle, p_TimeInfo);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set RTC alarm: %d", Error);

        return Error;
    }

    ESP_LOGI(TAG, "RTC alarm set for %02d:%02d", p_TimeInfo->tm_hour, p_TimeInfo->tm_min);

    return ESP_OK;
}

esp_err_t TimeManager_ClearAlarm(void)
{
    esp_err_t Error;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    }

    Error = BM8563_ClearAlarm(&_TimeManager_State.RTC_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear RTC alarm: %d!", Error);

        return Error;
    }

    ESP_LOGI(TAG, "RTC alarm cleared");

    return ESP_OK;
}

esp_err_t TimeManager_ClearAlarmFlag(void)
{
    esp_err_t Error;

    if (_TimeManager_State.isInitialized == false) {
        ESP_LOGE(TAG, "Time manager not initialized!");

        return ESP_ERR_INVALID_STATE;
    }

    Error = BM8563_ClearAlarmFlag(&_TimeManager_State.RTC_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear RTC alarm flag: %d!", Error);

        return Error;
    }

    ESP_LOGI(TAG, "RTC alarm flag cleared");

    return ESP_OK;
}

bool TimeManager_IsAlarmActive(void)
{
    if (_TimeManager_State.isInitialized == false) {
        return false;
    }

    return BM8563_IsAlarmActive(&_TimeManager_State.RTC_Handle);
}

bool TimeManager_IsSNTPSyncDue(void)
{
    if (_TimeManager_State.isInitialized == false) {
        return false;
    }

    /* If never synced, sync is due */
    if (_TimeManager_State.last_sync_time == 0) {
        ESP_LOGI(TAG, "SNTP sync due: never synced before");

        return true;
    }

    /* If sync interval is 0, never sync */
    if (_TimeManager_State.sync_interval_days == 0) {
        return false;
    }

    /* Get current time */
    time_t now = time(NULL);
    time_t time_since_sync = now - _TimeManager_State.last_sync_time;
    time_t sync_interval_seconds = _TimeManager_State.sync_interval_days * 24 * 60 * 60;

    bool is_due = (time_since_sync >= sync_interval_seconds);

    if (is_due) {
        ESP_LOGI(TAG, "SNTP sync due: %ld seconds since last sync (interval: %ld days)",
                 static_cast<long>(time_since_sync), static_cast<long>(_TimeManager_State.sync_interval_days));
    }

    return is_due;
}

bool TimeManager_IsRTCValid(void)
{
    if (_TimeManager_State.isInitialized == false) {
        return false;
    }

    return BM8563_IsValid(&_TimeManager_State.RTC_Handle);
}
