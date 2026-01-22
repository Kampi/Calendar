/*
 * settingsManager.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Persistent settings management using NVS storage.
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
#include <esp_event.h>
#include <esp_system.h>
#include <esp_mac.h>
#include <esp_efuse.h>
#include <esp_littlefs.h>

#include <nvs_flash.h>
#include <nvs.h>

#include <string.h>
#include <sys/stat.h>
#include <cJSON.h>

#include "settingsManager.h"
#include "Private/settingsLoader.h"

static const char *TAG = "settings_manager";

ESP_EVENT_DEFINE_BASE(SETTINGS_EVENTS);

static SettingsManager_State_t _State;

esp_err_t SettingsManager_Init(void)
{
    esp_err_t Error;

    if (_State.isInitialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Initializing Settings Manager...");

    ESP_ERROR_CHECK(nvs_flash_init());

    Error = nvs_flash_init_partition("settings");
    if ((Error == ESP_ERR_NVS_NO_FREE_PAGES) || (Error == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_LOGW(TAG, "Settings partition needs erase, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase_partition("settings"));
        Error = nvs_flash_init_partition("settings");
    } else if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize settings partition: %d!", Error);
        return Error;
    }

    Error = nvs_open_from_partition("settings", CONFIG_SETTINGS_NAMESPACE, NVS_READWRITE, &_State.NVS_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %d!", Error);

        return Error;
    }

    _State.isInitialized = true;

    Error = SettingsManager_LoadDefaultsFromJSON(&_State);
    if (Error != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load settings from file, using built-in defaults");
        SettingsManager_InitDefaults(&_State);
    }

    ESP_LOGD(TAG, "Settings Manager initialized");

    return ESP_OK;
}

esp_err_t SettingsManager_Deinit(void)
{
    if (_State.isInitialized == false) {
        return ESP_OK;
    }

    nvs_close(_State.NVS_Handle);

    _State.isInitialized = false;

    ESP_LOGD(TAG, "Settings Manager deinitialized");

    return ESP_OK;
}

esp_err_t SettingsManager_Load(App_Settings_t *p_Settings)
{
    esp_err_t Error;
    size_t RequiredSize;

    if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    } else if (p_Settings == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    Error = nvs_get_blob(_State.NVS_Handle, "settings", NULL, &RequiredSize);
    if (Error == ESP_ERR_NVS_NOT_FOUND) {

        return ESP_ERR_NVS_NOT_FOUND;
    } else if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get settings size: %d!", Error);
        return Error;
    }

    if (RequiredSize != sizeof(App_Settings_t)) {
        ESP_LOGW(TAG, "Settings size mismatch (expected %u, got %u), erasing and using defaults",
                 sizeof(App_Settings_t), RequiredSize);

        /* Erase the old settings */
        nvs_erase_key(_State.NVS_Handle, "settings");
        nvs_commit(_State.NVS_Handle);

        return ESP_ERR_INVALID_SIZE;
    }

    Error = nvs_get_blob(_State.NVS_Handle, "settings", &_State.Settings, &RequiredSize);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read settings: %d!", Error);

        return Error;
    }

    ESP_LOGD(TAG, "Settings loaded from NVS");

    return ESP_OK;
}

esp_err_t SettingsManager_GetWiFi(App_Settings_WiFi_t* p_Settings)
{
    if ( p_Settings == NULL ) {
        return ESP_ERR_INVALID_ARG;
    } else if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(p_Settings, &_State.Settings.WiFi, sizeof(App_Settings_WiFi_t));

    return ESP_OK;
}

esp_err_t SettingsManager_GetSystem(App_Settings_System_t* p_Settings)
{
    if ( p_Settings == NULL ) {
        return ESP_ERR_INVALID_ARG;
    } else if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(p_Settings, &_State.Settings.System, sizeof(App_Settings_System_t));

    return ESP_OK;
}

esp_err_t SettingsManager_GetCalDAV(App_Settings_CalDAV_t* p_Settings)
{
    if ( p_Settings == NULL ) {
        return ESP_ERR_INVALID_ARG;
    } else if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(p_Settings, &_State.Settings.CalDAV, sizeof(App_Settings_CalDAV_t));

    return ESP_OK;
}

void SettingsManager_GetCalendars(std::list<std::string>* p_Calendars)
{
    std::copy(_State.Settings.Calendars.begin(), _State.Settings.Calendars.end(), std::back_inserter(*p_Calendars));
}
