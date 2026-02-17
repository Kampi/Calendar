/*
 * settingsJSONLoader.cpp
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

#include <esp_log.h>
#include <esp_littlefs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <cJSON.h>

#include <new>

#include "settingsLoader.h"
#include "../settingsManager.h"

static const char *TAG = "settings_json_loader";

/** @brief
 *  @param p_State
 *  @param p_JSON
 */
static void SettingsManager_LoadWiFi(SettingsManager_State_t *p_State, const cJSON *p_JSON)
{
    cJSON *wifi = NULL;

    wifi = cJSON_GetObjectItem(p_JSON, "wifi");
    if (wifi != NULL) {
        cJSON *maxRetries = cJSON_GetObjectItem(wifi, "maxRetries");
        if (cJSON_IsNumber(maxRetries)) {
            p_State->Settings.WiFi.MaxRetries = (uint8_t)(maxRetries->valueint);
        } else {
            p_State->Settings.WiFi.MaxRetries = SETTINGS_WIFI_DEFAULT_MAX_RETRIES;
        }

        cJSON *timeout = cJSON_GetObjectItem(wifi, "timeout");
        if (cJSON_IsNumber(timeout)) {
            p_State->Settings.WiFi.TimeoutMs = (uint32_t)(timeout->valueint);
        } else {
            p_State->Settings.WiFi.TimeoutMs = SETTINGS_WIFI_DEFAULT_TIMEOUT_MS;
        }

        cJSON *ssid = cJSON_GetObjectItem(wifi, "ssid");
        if (cJSON_IsString(ssid)) {
            strncpy(p_State->Settings.WiFi.SSID, ssid->valuestring, sizeof(p_State->Settings.WiFi.SSID));
        } else {
            strncpy(p_State->Settings.WiFi.SSID, SETTINGS_WIFI_DEFAULT_SSID, sizeof(p_State->Settings.WiFi.SSID));
        }

        cJSON *password = cJSON_GetObjectItem(wifi, "password");
        if (cJSON_IsString(password)) {
            strncpy(p_State->Settings.WiFi.Password, password->valuestring, sizeof(p_State->Settings.WiFi.Password));
        } else {
            strncpy(p_State->Settings.WiFi.Password, SETTINGS_WIFI_DEFAULT_PASSWORD, sizeof(p_State->Settings.WiFi.Password));
        }
    } else {
        SettingsManager_InitDefaultWiFi(&p_State->Settings);
    }
}

/** @brief
 *  @param p_State
 *  @param p_JSON
 */
static void SettingsManager_LoadSystem(SettingsManager_State_t *p_State, const cJSON *p_JSON)
{
    cJSON *system = NULL;

    system = cJSON_GetObjectItem(p_JSON, "system");
    if (system != NULL) {
        cJSON *timezone = cJSON_GetObjectItem(system, "timezone");
        if (cJSON_IsString(timezone)) {
            strncpy(p_State->Settings.System.Timezone, timezone->valuestring, sizeof(p_State->Settings.System.Timezone));
        } else {
            strncpy(p_State->Settings.System.Timezone, SETTINGS_SYSTEM_DEFAULT_TIMEZONE, sizeof(p_State->Settings.System.Timezone));
        }

        cJSON *ntpServer = cJSON_GetObjectItem(system, "ntp-server");
        if (cJSON_IsString(ntpServer)) {
            strncpy(p_State->Settings.System.NTP_Server, ntpServer->valuestring, sizeof(p_State->Settings.System.NTP_Server));
        } else {
            strncpy(p_State->Settings.System.NTP_Server, SETTINGS_SYSTEM_DEFAULT_NTP_SERVER,
                    sizeof(p_State->Settings.System.NTP_Server));
        }

        cJSON *ntpSyncInterval = cJSON_GetObjectItem(system, "ntp-sync-interval");
        if (cJSON_IsNumber(ntpSyncInterval)) {
            p_State->Settings.System.NTP_SyncInterval = (uint32_t)(ntpSyncInterval->valueint);
        } else {
            p_State->Settings.System.NTP_SyncInterval = SETTINGS_SYSTEM_DEFAULT_NTP_SYNC_INTERVAL;
        }

        cJSON *sleepDurationHours = cJSON_GetObjectItem(system, "sleep-hours");
        if (cJSON_IsNumber(sleepDurationHours)) {
            p_State->Settings.System.SleepDurationHours = (uint8_t)(sleepDurationHours->valueint);
        } else {
            p_State->Settings.System.SleepDurationHours = SETTINGS_SYSTEM_DEFAULT_SLEEP_DURATION_HOURS;
        }

        cJSON *ntpTimeout = cJSON_GetObjectItem(system, "ntp-timeout");
        if (cJSON_IsNumber(ntpTimeout)) {
            p_State->Settings.System.NTP_Timeout = (uint32_t)(ntpTimeout->valueint);
        } else {
            p_State->Settings.System.NTP_Timeout = SETTINGS_SYSTEM_DEFAULT_NTP_TIMEOUT;
        }
    } else {
        SettingsManager_InitDefaultSystem(&p_State->Settings);
    }
}

/** @brief
 *  @param p_State
 *  @param p_JSON
 */
static void SettingsManager_LoadCalDAV(SettingsManager_State_t *p_State, const cJSON *p_JSON)
{
    cJSON *caldav = NULL;

    caldav = cJSON_GetObjectItem(p_JSON, "caldav");
    if (caldav != NULL) {
        cJSON *url = cJSON_GetObjectItem(caldav, "url");
        if (cJSON_IsString(url)) {
            strncpy(p_State->Settings.CalDAV.URL, url->valuestring, sizeof(p_State->Settings.CalDAV.URL));
        } else {
            strncpy(p_State->Settings.CalDAV.URL, "", sizeof(p_State->Settings.CalDAV.URL));
        }

        cJSON *username = cJSON_GetObjectItem(caldav, "username");
        if (cJSON_IsString(username)) {
            strncpy(p_State->Settings.CalDAV.Username, username->valuestring, sizeof(p_State->Settings.CalDAV.Username));
        } else {
            strncpy(p_State->Settings.CalDAV.Username, "", sizeof(p_State->Settings.CalDAV.Username));
        }

        cJSON *password = cJSON_GetObjectItem(caldav, "password");
        if (cJSON_IsString(password)) {
            strncpy(p_State->Settings.CalDAV.Password, password->valuestring, sizeof(p_State->Settings.CalDAV.Password));
        } else {
            strncpy(p_State->Settings.CalDAV.Password, "", sizeof(p_State->Settings.CalDAV.Password));
        }

        cJSON *calendars = cJSON_GetObjectItem(caldav, "calendars");
        if (cJSON_IsArray(calendars)) {
            /* Use placement new to construct list in uninitialized memory */
            new (&p_State->Settings.Calendars) std::list<std::string>();
            cJSON *calendar = NULL;
            cJSON_ArrayForEach(calendar, calendars) {
                if (cJSON_IsString(calendar)) {
                    p_State->Settings.Calendars.push_back(std::string(calendar->valuestring));
                }
            }
        } else {
            /* Use placement new to construct empty list */
            new (&p_State->Settings.Calendars) std::list<std::string>();
        }
    } else {
        SettingsManager_InitDefaultCalDAV(&p_State->Settings);
    }
}

/** @brief  Initialize and mount the SD card.
 *  @return ESP_OK on success
 */
static esp_err_t SettingsManager_Mount_SD_Card(void)
{
    esp_err_t Error;
    sdmmc_card_t *card;
    const char mount_point[] = "/sdcard";

    ESP_LOGD(TAG, "Initializing SD card");

    sdmmc_host_t Host = SDSPI_HOST_DEFAULT();
    Host.slot = SPI2_HOST;

    spi_bus_config_t SPI_Config = {
        .mosi_io_num = GPIO_NUM_38,
        .miso_io_num = GPIO_NUM_40,
        .sclk_io_num = GPIO_NUM_39,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    Error = spi_bus_initialize(static_cast<spi_host_device_t>(Host.slot), &SPI_Config, SDSPI_DEFAULT_DMA);
    if (Error != ESP_OK) {
        if (Error == ESP_ERR_INVALID_STATE) {
            ESP_LOGD(TAG, "SPI bus already initialized, continuing...");
        } else {
            ESP_LOGD(TAG, "Failed to initialize SPI: %d!", Error);

            return Error;
        }
    }

    sdspi_device_config_t Slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    Slot.gpio_cs = GPIO_NUM_47;
    Slot.host_id = static_cast<spi_host_device_t>(Host.slot);

    esp_vfs_fat_sdmmc_mount_config_t Mount_Config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    Error = esp_vfs_fat_sdspi_mount(mount_point, &Host, &Slot, &Mount_Config, &card);
    if (Error != ESP_OK) {
        if (Error == ESP_FAIL) {
            ESP_LOGD(TAG, "Failed to mount SD card filesystem!");
        } else {
            ESP_LOGD(TAG, "Failed to mount SD card: 0x%x", Error);
        }

        return Error;
    }

    /* Log the mount point and list files in the directory */
    DIR *dir = opendir(mount_point);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open mount point: %s", mount_point);
        return ESP_FAIL;
    }

    struct dirent *entry;
    ESP_LOGD(TAG, "Files in mount point %s:", mount_point);
    /* Enhanced logging for file names */
    while ((entry = readdir(dir)) != NULL) {
        ESP_LOGD(TAG, "  Found file: %s", entry->d_name);
        if (strcmp(entry->d_name, "settings.json") == 0 || strcmp(entry->d_name, "SETTIN~1.JSO") == 0) {
            ESP_LOGD(TAG, "  Matched settings file: %s", entry->d_name);
        }
    }
    closedir(dir);

    ESP_LOGD(TAG, "SD card mounted successfully");

    return ESP_OK;
}

/** @brief Unmount the SD card.
 */
static void SettingsManager_Unmount_SD_Card(void)
{
    esp_vfs_fat_sdcard_unmount("/sdcard", NULL);

    spi_bus_free(static_cast<spi_host_device_t>(SPI2_HOST));

    ESP_LOGD(TAG, "SD card unmounted");
}

/** @brief          Load and parse JSON settings from file.
 *  @param p_State  Settings state structure
 *  @param filepath Full path to JSON file
 *  @return         ESP_OK on success
 */
static esp_err_t SettingsManager_Load_JSON(SettingsManager_State_t *p_State, const char *p_FilePath)
{
    FILE *File = NULL;
    char *Buffer = NULL;
    long FileSize;
    size_t BytesRead;
    cJSON *JSON = NULL;

    ESP_LOGD(TAG, "Loading settings from: %s", p_FilePath);

    /* Check if file exists */
    struct stat st;
    if (stat(p_FilePath, &st) != 0) {
        ESP_LOGE(TAG, "File does not exist or cannot be accessed: %s", p_FilePath);
        return ESP_ERR_NOT_FOUND;
    }

    File = fopen(p_FilePath, "r");
    if (File == NULL) {
        ESP_LOGW(TAG, "Failed to open: %s", p_FilePath);

        return ESP_ERR_NOT_FOUND;
    }

    /* Get file size */
    fseek(File, 0, SEEK_END);
    FileSize = ftell(File);
    fseek(File, 0, SEEK_SET);

    if (FileSize <= 0) {
        ESP_LOGE(TAG, "Invalid file size: %ld!", FileSize);

        fclose(File);

        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGD(TAG, "File size: %ld bytes", FileSize);

    /* Allocate buffer */
    Buffer = static_cast<char *>(heap_caps_malloc(FileSize + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (Buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %ld bytes", FileSize);

        fclose(File);

        return ESP_ERR_NO_MEM;
    }

    /* Read file */
    BytesRead = fread(Buffer, 1, FileSize, File);
    fclose(File);
    Buffer[FileSize] = '\0';

    if (BytesRead != FileSize) {
        ESP_LOGE(TAG, "Read error: got %zu of %ld bytes!", BytesRead, FileSize);

        heap_caps_free(Buffer);

        return ESP_FAIL;
    }

    /* Parse JSON */
    JSON = cJSON_Parse(Buffer);
    heap_caps_free(Buffer);

    if (JSON == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();

        ESP_LOGE(TAG, "JSON parse error: %s!", error_ptr ? error_ptr : "unknown");

        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "JSON parsed successfully from %s", p_FilePath);

    /* Load settings sections */
    SettingsManager_LoadWiFi(p_State, JSON);
    SettingsManager_LoadSystem(p_State, JSON);
    SettingsManager_LoadCalDAV(p_State, JSON);

    cJSON_Delete(JSON);

    return ESP_OK;
}

esp_err_t SettingsManager_LoadDefaultsFromJSON(SettingsManager_State_t *p_State)
{
    esp_err_t Error;
    esp_vfs_littlefs_conf_t LittleFS_Config = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .partition = NULL,
        .format_if_mount_failed = true,
        .read_only = true,
        .dont_mount = false,
        .grow_on_mount = false
    };

    if (p_State == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Loading settings with priority: SD Card -> LittleFS -> Built-in defaults");

    /* 1. Try SD card */
    Error = SettingsManager_Mount_SD_Card();
    if (Error == ESP_OK) {
        Error = SettingsManager_Load_JSON(p_State, "/sdcard/settings.json");

        /* Check for 8.3 filename match */
        if (Error != ESP_OK) {
            ESP_LOGW(TAG, "Falling back to 8.3 filename: SETTIN~1.JSO");
            Error = SettingsManager_Load_JSON(p_State, "/sdcard/SETTIN~1.JSO");
        }

        SettingsManager_Unmount_SD_Card();

        if (Error == ESP_OK) {
            ESP_LOGD(TAG, "Settings loaded from SD card");
            return ESP_OK;
        }
        ESP_LOGW(TAG, "SD card mounted but no valid settings.json found");
    } else {
        ESP_LOGD(TAG, "SD card not available, trying LittleFS");
    }

    /* 2. Try LittleFS */
    Error = esp_vfs_littlefs_register(&LittleFS_Config);
    if (Error == ESP_OK) {
        Error = SettingsManager_Load_JSON(p_State, "/littlefs/settings.json");
        esp_vfs_littlefs_unregister(LittleFS_Config.partition_label);

        if (Error == ESP_OK) {
            ESP_LOGD(TAG, "Settings loaded from LittleFS");
            return ESP_OK;
        }
        ESP_LOGW(TAG, "LittleFS mounted but no valid settings.json found");
    } else {
        ESP_LOGW(TAG, "Failed to mount LittleFS: %d!", Error);
    }

    /* 3. Fallback to built-in defaults */
    ESP_LOGW(TAG, "Using built-in default settings");
    SettingsManager_InitDefaults(p_State);

    return ESP_OK;
}