/*
 * settingsDefaultLoader.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Default settings loader implementation.
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

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_efuse.h>

#include <nvs_flash.h>
#include <nvs.h>

#include <string.h>

#include "settingsLoader.h"
#include "../settingsManager.h"

static const char *TAG = "settings_default_loader";

void SettingsManager_InitDefaults(SettingsManager_State_t *p_State)
{
    memset(&p_State->Settings, 0, sizeof(App_Settings_t));

    SettingsManager_InitDefaultWiFi(&p_State->Settings);
    SettingsManager_InitDefaultSystem(&p_State->Settings);
    SettingsManager_InitDefaultCalDAV(&p_State->Settings);
}

void SettingsManager_InitDefaultWiFi(App_Settings_t *p_Settings)
{
    ESP_LOGW(TAG, "Loading default WiFi settings");

    p_Settings->WiFi.MaxRetries = SETTINGS_WIFI_DEFAULT_MAX_RETRIES;
    p_Settings->WiFi.TimeoutMs = SETTINGS_WIFI_DEFAULT_TIMEOUT_MS;
    strncpy(p_Settings->WiFi.SSID, SETTINGS_WIFI_DEFAULT_SSID, sizeof(p_Settings->WiFi.SSID));
    strncpy(p_Settings->WiFi.Password, SETTINGS_WIFI_DEFAULT_PASSWORD, sizeof(p_Settings->WiFi.Password));
}

void SettingsManager_InitDefaultSystem(App_Settings_t *p_Settings)
{
    ESP_LOGW(TAG, "Loading default System settings");

    strncpy(p_Settings->System.Timezone, SETTINGS_SYSTEM_DEFAULT_TIMEZONE, sizeof(p_Settings->System.Timezone));
    strncpy(p_Settings->System.NTP_Server, SETTINGS_SYSTEM_DEFAULT_NTP_SERVER, sizeof(p_Settings->System.NTP_Server));
    p_Settings->System.NTP_SyncInterval = SETTINGS_SYSTEM_DEFAULT_NTP_SYNC_INTERVAL;
    p_Settings->System.SleepDurationHours = SETTINGS_SYSTEM_DEFAULT_SLEEP_DURATION_HOURS;
}

void SettingsManager_InitDefaultCalDAV(App_Settings_t *p_Settings)
{
    ESP_LOGW(TAG, "Loading default CalDAV settings");

    strncpy(p_Settings->CalDAV.URL, SETTINGS_CALDAV_DEFAULT_URL, sizeof(p_Settings->CalDAV.URL));
    strncpy(p_Settings->CalDAV.Username, SETTINGS_CALDAV_DEFAULT_USERNAME, sizeof(p_Settings->CalDAV.Username));
    strncpy(p_Settings->CalDAV.Password, SETTINGS_CALDAV_DEFAULT_PASSWORD, sizeof(p_Settings->CalDAV.Password));
    p_Settings->CalDAV.TimeoutMs = SETTINGS_CALDAV_DEFAULT_TIMEOUT_MS;
    new (&p_Settings->Calendars) std::list<std::string>();
}