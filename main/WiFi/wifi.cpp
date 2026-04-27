
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <string.h>

#include "wifi.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t _WiFi_EventGroup;
static int _WiFi_RetryNum = 0;
static bool _WiFi_IsConnected = false;
static uint8_t _WiFi_MaxRetries;

static const char *TAG = "WiFi";

static void on_WiFi_Event(void *p_Arg, esp_event_base_t EventBase,
                          int32_t EventId, void *p_EventData)
{
    if (EventBase == WIFI_EVENT) {
        if (EventId == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (EventId == WIFI_EVENT_STA_DISCONNECTED) {
            _WiFi_IsConnected = false;
            if (_WiFi_RetryNum < _WiFi_MaxRetries) {
                esp_wifi_connect();
                _WiFi_RetryNum++;
            } else {
                xEventGroupSetBits(_WiFi_EventGroup, WIFI_FAIL_BIT);
            }
        }
    }

    if (EventBase == IP_EVENT) {
        if (EventId == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *p_Event = static_cast<ip_event_got_ip_t *>(p_EventData);
            ESP_LOGD(TAG, "IP adresse received: " IPSTR, IP2STR(&p_Event->ip_info.ip));
            _WiFi_RetryNum = 0;
            _WiFi_IsConnected = true;
            xEventGroupSetBits(_WiFi_EventGroup, WIFI_CONNECTED_BIT);
        }
    }
}

esp_err_t WiFi_Init(const char* SSID, const char* Password, uint8_t Retries, uint32_t TimeoutMs)
{
    wifi_config_t WiFiConfig;
    wifi_init_config_t WiFiInitConfig = WIFI_INIT_CONFIG_DEFAULT();

    _WiFi_MaxRetries = Retries;

    _WiFi_EventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_init(&WiFiInitConfig));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_WiFi_Event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_WiFi_Event, NULL));

    memset(&WiFiConfig, 0, sizeof(wifi_config_t));

    strlcpy(reinterpret_cast<char *>(WiFiConfig.sta.ssid), SSID, sizeof(WiFiConfig.sta.ssid));
    strlcpy(reinterpret_cast<char *>(WiFiConfig.sta.password), Password, sizeof(WiFiConfig.sta.password));

    WiFiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    WiFiConfig.sta.pmf_cfg.capable = true;
    WiFiConfig.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &WiFiConfig));

    /* Enable WiFi maximum power save mode to reduce power consumption during WiFi operations */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));

    ESP_ERROR_CHECK(esp_wifi_start());

    /* Wait for connection or error */
    TickType_t Timeout = (TimeoutMs == 0) ? portMAX_DELAY : pdMS_TO_TICKS(TimeoutMs);

    ESP_LOGD(TAG, "Waiting for WiFi connection (timeout: %lu ms)...", TimeoutMs);

    EventBits_t Bits = xEventGroupWaitBits(_WiFi_EventGroup,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           Timeout);

    if (Bits & WIFI_CONNECTED_BIT) {
        ESP_LOGD(TAG, "WiFi connected successfully");
        return ESP_OK;
    } else if (Bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi connection failed after %d retries", _WiFi_MaxRetries);
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, "WiFi connection timeout after %lu ms", TimeoutMs);

    return ESP_ERR_TIMEOUT;
}

bool WiFi_IsConnected(void)
{
    return _WiFi_IsConnected;
}

esp_err_t WiFi_Deinit(void)
{
    esp_err_t Error;

    ESP_LOGD(TAG, "Disconnecting and deinitializing WiFi...");

    Error = esp_wifi_disconnect();
    if (Error != ESP_OK) {
        ESP_LOGW(TAG, "WiFi disconnect failed: 0x%x!", Error);
    }

    Error = esp_wifi_stop();
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "WiFi stop failed: 0x%x!", Error);
        return Error;
    }

    Error = esp_wifi_deinit();
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "WiFi deinit failed: 0x%x!", Error);
        return Error;
    }

    /* Unregister event handlers */
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_WiFi_Event);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_WiFi_Event);

    /* Delete event group to free resources */
    if (_WiFi_EventGroup != NULL) {
        vEventGroupDelete(_WiFi_EventGroup);
        _WiFi_EventGroup = NULL;
    }

    /* Deinitialize network interface and event loop */
    Error = esp_netif_deinit();
    if (Error != ESP_OK) {
        ESP_LOGW(TAG, "Netif deinit failed: 0x%x!", Error);
    }

    Error = esp_event_loop_delete_default();
    if (Error != ESP_OK) {
        ESP_LOGW(TAG, "Event loop delete failed: 0x%x!", Error);
    }

    _WiFi_IsConnected = false;
    ESP_LOGD(TAG, "WiFi deinitialized successfully");

    return ESP_OK;
}

int8_t WiFi_GetLastRSSI(void)
{
    wifi_ap_record_t APRecord;
    esp_err_t Error;

    if (_WiFi_IsConnected == false) {
        ESP_LOGW(TAG, "WiFi is not connected, cannot get RSSI");
        return 0;
    }

    Error = esp_wifi_sta_get_ap_info(&APRecord);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP info: 0x%x", Error);
        return 0;
    }

    return APRecord.rssi;
}