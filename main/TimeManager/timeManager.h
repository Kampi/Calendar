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

#endif /* TIMEMANAGER_H_ */
