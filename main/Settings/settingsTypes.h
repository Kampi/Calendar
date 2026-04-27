/*
 * settings_types.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Settings Manager event types and definitions.
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

#ifndef SETTINGS_TYPES_H_
#define SETTINGS_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string>
#include <list>

#include <esp_event.h>

/** @brief WiFi settings.
 */
typedef struct {
    char SSID[34];                              /**< WiFi SSID. */
    char Password[66];                          /**< WiFi password. */
    uint8_t MaxRetries;                         /**< Maximum number of connection retries. */
    uint32_t TimeoutMs;                         /**< Connection timeout in milliseconds (0 = wait forever). */
} App_Settings_WiFi_t;

/** @brief System settings.
 */
typedef struct {
    char Timezone[32];                          /**< Timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3"). */
    char NTP_Server[32];                        /**< NTP server address. */
    uint32_t NTP_SyncInterval;                  /**< NTP sync interval in days (0 = never). */
    uint8_t SleepDurationHours;                 /**< Deep sleep duration in hours. */
    uint32_t NTP_Timeout;                       /**< NTP fetch timeout in seconds */
} App_Settings_System_t;

/** @brief CalDAV settings.
 */
typedef struct {
    char URL[128];                              /**< CalDAV server URL. */
    char Username[64];                          /**< CalDAV username. */
    char Password[64];                          /**< CalDAV password. */
    uint32_t TimeoutMs;                         /**< CalDAV request timeout in milliseconds. */
} App_Settings_CalDAV_t;

/** @brief Complete application settings structure.
 */
typedef struct {
    App_Settings_WiFi_t WiFi;                   /**< WiFi settings. */
    App_Settings_System_t System;               /**< System settings. */
    App_Settings_CalDAV_t CalDAV;               /**< CalDAV settings. */
    std::list<std::string> Calendars;           /**< List of calendar names to fetch. */
} App_Settings_t;

#endif /* SETTINGS_TYPES_H_ */
