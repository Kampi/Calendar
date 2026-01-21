#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include "esp_err.h"

#include <stdbool.h>

//#define WIFI_SSID       "Daniel"
//#define WIFI_PASSWORD   "!!Kampert4052"

#define WIFI_SSID       "PrettyFlyForAWifi"
#define WIFI_PASSWORD   "pelikan89"

/**
 * @brief Initialisiert WiFi und verbindet sich mit dem konfigurierten Netzwerk
 * @return ESP_OK bei Erfolg, sonst Fehlercode
 */
esp_err_t wifi_connect_init(void);

/**
 * @brief Prüft ob WiFi verbunden ist
 * @return true wenn verbunden, false sonst
 */
bool wifi_is_connected(void);

#endif // WIFI_CONNECT_H
