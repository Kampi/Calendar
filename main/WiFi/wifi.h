#ifndef WIFI_H_
#define WIFI_H_

#include <esp_err.h>

#include <stdbool.h>

/** @brief              Initialize WiFi and connect to configured network
 *  @param SSID         SSID of the WiFi network
 *  @param Password     Password of the WiFi network
 *  @param Retries      Number of connection retries
 *  @param TimeoutMs    Connection timeout in milliseconds (0 = wait forever)
 *  @return             ESP_OK on success, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t WiFi_Init(char* SSID, char* Password, uint8_t Retries, uint32_t TimeoutMs);

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
