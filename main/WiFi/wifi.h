/*
 * wifi.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: WiFi management functions for connecting to and managing WiFi networks.
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

#ifndef WIFI_H_
#define WIFI_H_

#include <esp_err.h>

#include <stdint.h>
#include <stdbool.h>

/** @brief              Initialize WiFi and connect to configured network
 *  @param SSID         SSID of the WiFi network
 *  @param Password     Password of the WiFi network
 *  @param Retries      Number of connection retries
 *  @param TimeoutMs    Connection timeout in milliseconds (0 = wait forever)
 *  @return             ESP_OK on success, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t WiFi_Init(const char* SSID, const char* Password, uint8_t Retries, uint32_t TimeoutMs);

/** @brief  Check if WiFi is connected
 *  @return true if connected, false otherwise
 */
bool WiFi_IsConnected(void);

/** @brief  Deinitialize WiFi and disconnect to save power
 *  @return ESP_OK on success
 */
esp_err_t WiFi_Deinit(void);

/** @brief  Get last RSSI value.
 *  @return Last RSSI value in dBm
 */
int8_t WiFi_GetLastRSSI(void);

#endif /* WIFI_H_ */
