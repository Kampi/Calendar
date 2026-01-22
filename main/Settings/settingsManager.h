/*
 * settingsManager.h
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

#ifndef SETTINGS_MANAGER_H_
#define SETTINGS_MANAGER_H_

#include <esp_err.h>
#include <stdbool.h>

#include "settingsTypes.h"

/** @brief  Initialize the Settings Manager and load all settings from NVS into the Settings Manager RAM and into the provided structure.
 *  @return ESP_OK on success
 */
esp_err_t SettingsManager_Init(void);

/** @brief  Deinitialize the Settings Manager.
 *  @return ESP_OK on success
 */
esp_err_t SettingsManager_Deinit(void);

/** @brief              Load all settings from NVS into the Settings Manager RAM and into the provided structure. This function overwrites all unsaved settings in RAM.
 *  @param p_Settings   Pointer to settings structure to populate
 *  @return             ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no settings exist
 */
esp_err_t SettingsManager_Load(App_Settings_t *p_Settings);

/** @brief  Save all RAM settings to NVS.
 *  @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t SettingsManager_Save(void);

/** @brief              Get the WiFi settings from the Settings Manager RAM.
 *  @param p_Settings   Pointer to WiFi settings structure to populate
 *  @return             ESP_OK on success, ESP_ERR_* on failure
*/
esp_err_t SettingsManager_GetWiFi(App_Settings_WiFi_t* p_Settings);

/** @brief              Get the system settings from the Settings Manager RAM.
 *  @param p_Settings   Pointer to System settings structure to populate
 *  @return             ESP_OK on success, ESP_ERR_* on failure
*/
esp_err_t SettingsManager_GetSystem(App_Settings_System_t* p_Settings);

/** @brief              Get the caldav settings from the Settings Manager RAM.
 *  @param p_Settings   Pointer to CalDAV settings structure to populate
 *  @return             ESP_OK on success, ESP_ERR_* on failure
*/
esp_err_t SettingsManager_GetCalDAV(App_Settings_CalDAV_t* p_Settings);

/** @brief              Get the list of calendars to fetch from the Settings Manager RAM.
 *  @param p_Calendars  Pointer to the calendars to fetch
*/
void SettingsManager_GetCalendars(std::list<std::string>* p_Calendars);

#endif /* SETTINGS_MANAGER_H_ */
