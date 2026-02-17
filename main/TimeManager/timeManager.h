/*
 * timeManager.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Time manager with RTC and SNTP synchronization.
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

#ifndef TIMEMANAGER_H_
#define TIMEMANAGER_H_

#include <time.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_event.h>

/** @brief TimeManager events base.
 */
ESP_EVENT_DECLARE_BASE(TIMEMANAGER_EVENTS);

/** @brief TimeManager event IDs.
 */
enum {
    TIMEMANAGER_EVENT_ALARM_TRIGGERED,  /**< RTC alarm triggered */
};

/** @brief Time manager p_Configuration.
 */
typedef struct {
    char Timezone[64];                  /**< Timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3") */
    char NTPServer[128];                /**< NTP server address */
    uint32_t SyncInterval;              /**< SNTP sync interval in seconds (0 = no auto sync) */
    i2c_master_bus_handle_t BusHandle;  /**< I2C master bus handle for RTC */
} TimeManager_Config_t;

/** @brief          Initialize time manager
 *  @param p_Config Pointer to p_Configuration structure
 *  @return         ESP_OK on success
 */
esp_err_t TimeManager_Init(const TimeManager_Config_t *p_Config);

/** @brief  Deinitialize time manager
 *  @return ESP_OK on success
 */
esp_err_t TimeManager_Deinit(void);

/** @brief          Get current time
 *  @param p_TimeInfo Pointer to tm structure to store time
 *  @return         ESP_OK on success
 */
esp_err_t TimeManager_GetTime(struct tm *p_TimeInfo);

/** @brief          Set system time manually
 *  @param p_TimeInfo Pointer to tm structure with time to set
 *  @param SyncRTC  If true, also sync to RTC
 *  @return         ESP_OK on success
 */
esp_err_t TimeManager_SetTime(const struct tm *p_TimeInfo, bool SyncRTC);

/** @brief  Sync system time with RTC
 *  @return ESP_OK on success
 */
esp_err_t TimeManager_SyncFromRTC(void);

/** @brief              Sync system time with SNTP server
 *  @param TimeoutMs    Timeout in milliseconds (0 = use default)
 *  @return             ESP_OK on success
 */
esp_err_t TimeManager_SyncFromSNTP(uint32_t TimeoutMs);

/** @brief  Sync RTC with system time
 *  @return ESP_OK on success
 */
esp_err_t TimeManager_SyncToRTC(void);

/** @brief  Check if time is valid
 *  @return true if time has been set
 */
bool TimeManager_IsTimeValid(void);

/** @brief          Set RTC alarm time.
 *  @param p_TimeInfo Pointer to tm structure with alarm time
 *                  tm_hour and tm_min are used for time
 *                  tm_mday and tm_wday can be used for specific day/weekday (-1 to ignore)
 *  @return         ESP_OK on success
 */
esp_err_t TimeManager_SetAlarm(const struct tm *p_TimeInfo);

/** @brief  Disable RTC alarm.
 *  @return ESP_OK on success
 */
esp_err_t TimeManager_ClearAlarm(void);

/** @brief  Clear RTC alarm interrupt flag.
 *          Must be called after alarm triggers to re-enable alarm.
 *  @return ESP_OK on success
 */
esp_err_t TimeManager_ClearAlarmFlag(void);

/** @brief  Check if RTC alarm has triggered.
 *  @return true if alarm flag is set
 */
bool TimeManager_IsAlarmActive(void);

#endif /* TIMEMANAGER_H_ */
