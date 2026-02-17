/*
 * settingsLoader.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: JSON settings loader for factory defaults.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Errors and commissions should be reported to DanielKampert@kampis-elektroecke.de
 */

#ifndef SETTINGS_LOADER_H_
#define SETTINGS_LOADER_H_

#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <nvs_flash.h>
#include <nvs.h>

#include <stdbool.h>

#include "../settingsTypes.h"

#define SETTINGS_SYSTEM_DEFAULT_TIMEZONE                "CET-1CEST,M3.5.0,M10.5.0/3"
#define SETTINGS_SYSTEM_DEFAULT_NTP_SERVER              "pool.ntp.org"
#define SETTINGS_SYSTEM_DEFAULT_NTP_SYNC_INTERVAL       2
#define SETTINGS_SYSTEM_DEFAULT_SLEEP_DURATION_HOURS    3
#define SETTINGS_SYSTEM_DEFAULT_NTP_TIMEOUT             5

#define SETTINGS_WIFI_DEFAULT_SSID                      ""
#define SETTINGS_WIFI_DEFAULT_PASSWORD                  ""
#define SETTINGS_WIFI_DEFAULT_MAX_RETRIES               5
#define SETTINGS_WIFI_DEFAULT_TIMEOUT_MS                5000

#define SETTINGS_CALDAV_DEFAULT_URL                     ""
#define SETTINGS_CALDAV_DEFAULT_USERNAME                ""
#define SETTINGS_CALDAV_DEFAULT_PASSWORD                ""
#define SETTINGS_CALDAV_DEFAULT_TIMEOUT_MS              5000

/** @brief Settings Manager state.
 */
typedef struct {
    bool isInitialized;
    nvs_handle_t NVS_Handle;
    App_Settings_t Settings;
} SettingsManager_State_t;

/** @brief          Initialize the settings presets from the NVS by using a config JSON file.
 *  @param p_State  Pointer to the Settings Manager state structure
 *  @return         ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t SettingsManager_LoadDefaultsFromJSON(SettingsManager_State_t *p_State);

/** @brief          Initialize settings with factory defaults.
 *  @param p_State  Pointer to Settings Manager state structure
 */
void SettingsManager_InitDefaults(SettingsManager_State_t *p_State);

/** @brief              Initialize WiFi settings with factory defaults.
 *  @param p_Settings   Pointer to settings structure
 */
void SettingsManager_InitDefaultWiFi(App_Settings_t *p_Settings);

/** @brief              Initialize System settings with factory defaults.
 *  @param p_Settings   Pointer to settings structure
 */
void SettingsManager_InitDefaultSystem(App_Settings_t *p_Settings);

/** @brief              Initialize CalDAV settings with factory defaults.
 *  @param p_Settings   Pointer to settings structure
 */
void SettingsManager_InitDefaultCalDAV(App_Settings_t *p_Settings);

#endif /* SETTINGS_LOADER_H_ */