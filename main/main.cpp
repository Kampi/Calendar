#include <esp_log.h>

#include <lvgl.h>

#include <stdio.h>

#include "bsp/esp-bsp.h"
#include "wifi_connect.h"

#include "UI/ui.h"
#include "CalDAV/caldav.h"

CalDAV_Client_t *Client = NULL;

static const char *TAG = "main";

extern "C" void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = {
            .task_priority = 4,
            .task_stack = 12288,  // Increased from 7168 to 12KB for complex grid layouts
            .task_affinity = -1,
            .task_max_sleep_ms = 500,
            .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT,
            .timer_period_ms = 5,
        }
    };
    bsp_display_start_with_config(&cfg);

    bsp_display_lock(0);
#if (CONFIG_ESP_LCD_TOUCH_MAX_POINTS > 1 && CONFIG_LV_USE_GESTURE_RECOGNITION)
    lv_indev_t *indev = bsp_display_get_input_dev();
    lv_indev_set_rotation_rad_threshold(indev, 0.15f);
#endif

    bsp_display_unlock();
    bsp_display_backlight_on();

    UI_Dashboard_Create(7, 21);
    UI_Dashboard_Add_Event("today", 9, 
                       "Product Photography", 
                       "09:00 AM", "10:30 AM",
                       "Studio A", 
                       "Photo session with comments and review");
    ESP_LOGI(TAG, "Starte WiFi-Verbindung...");
    wifi_connect_init();
    while (wifi_is_connected() == false) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "WiFi verbunden!");

    Client = CalDAV_Init();
    if (Client != NULL) {
        CalDAV_List_Calendars(Client);
        CalDAV_List_Calendar_Events(Client, "siaesz");
        CalDAV_Deinit(Client);
    }

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
