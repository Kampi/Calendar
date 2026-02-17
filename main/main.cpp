/*
 * main.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Calendar application main file.
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
#include <esp_sleep.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <esp_lcd_touch_gt911.h>

#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>

#include <driver/gpio.h>

#include <lvgl.h>
#include <FastEPD.h>
#include <caldav_client.h>

#include "EventManager/eventManager.h"
#include "UI/ui.h"
#include "UI/eventView.h"
#include "SNTP/sntp.h"
#include "WiFi/wifi.h"
#include "TimeManager/timeManager.h"
#include "Devices/devices.h"
#include "Settings/settingsManager.h"

#define GUI_DRAW_BUFFER_SIZE                ((((CONFIG_GUI_WIDTH * CONFIG_GUI_HEIGHT) / 10 * LV_COLOR_DEPTH) / 8) + 8)

#ifdef CONFIG_GUI_VIEW_LIST
#define GUI_VIEW_CREATE(...)            UI_DayView_Create(__VA_ARGS__)
#define GUI_VIEW_UPDATE_HEADER(...)     UI_DayView_Update_Header(__VA_ARGS__)
#define GUI_VIEW_UPDATE_BATTERY(...)    UI_DayView_Update_Battery(__VA_ARGS__)
#define GUI_VIEW_UPDATE_RSSI(...)       UI_DayView_Update_RSSI(__VA_ARGS__)
#define GUI_VIEW_ADD_EVENT(...)         UI_DayView_Add_Event(__VA_ARGS__)
#define GUI_VIEW_CLEAR_EVENTS(...)      UI_DayView_Clear_Events(__VA_ARGS__)
#define GUI_VIEW_DESTROY(...)           UI_DayView_Destroy(__VA_ARGS__)
#elif CONFIG_GUI_VIEW_CALENDAR
#define GUI_VIEW_CREATE(...)            UI_Dashboard_Create(__VA_ARGS__)
#define GUI_VIEW_UPDATE_HEADER(...)     UI_Dashboard_Update_Header(__VA_ARGS__)
#define GUI_VIEW_UPDATE_BATTERY(...)    UI_Dashboard_Update_Battery(__VA_ARGS__)
#define GUI_VIEW_UPDATE_RSSI(...)       UI_Dashboard_Update_RSSI(__VA_ARGS__)
#define GUI_VIEW_ADD_EVENT(...)         UI_Dashboard_Add_Event(__VA_ARGS__)
#define GUI_VIEW_CLEAR_EVENTS(...)      UI_Dashboard_Clear_Events(__VA_ARGS__)
#define GUI_VIEW_DESTROY(...)           UI_Dashboard_Destroy(__VA_ARGS__)
#else
#error "No GUI view defined!"
#endif

static CalDAV_Client_t Client;
static FASTEPD epaper;
static EventView_Handle_t *Event_View;
static uint16_t *LVGL_DrawBuffer;
static esp_lcd_touch_handle_t Touch_Handle;
static esp_lcd_panel_io_handle_t IO_Handle;
static lv_indev_t *Touch;
static lv_display_t *Display;
static lv_obj_t *Screen_Status;
static lv_obj_t *Screen_Dashboard;
static i2c_master_bus_handle_t I2C_Bus_Handle;
static i2c_master_bus_config_t I2CM_Config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_41,
    .scl_io_num = GPIO_NUM_42,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .trans_queue_depth = 0,
    .flags = {
        .enable_internal_pullup = false,
        .allow_pd = false,
    },
};
static esp_lcd_touch_config_t Touch_Config = {
    .x_max = CONFIG_GUI_WIDTH,
    .y_max = CONFIG_GUI_HEIGHT,
    .rst_gpio_num = GPIO_NUM_NC,
    .int_gpio_num = GPIO_NUM_48,
    .levels = {
        .reset = 0,
        .interrupt = 0,
    },
    .flags = {
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    },
};
static esp_lcd_panel_io_i2c_config_t I2C_Touch_Config = {
    .dev_addr = 0x5D,
    .on_color_trans_done = NULL,
    .user_ctx = NULL,
    .control_phase_bytes = 1,
    .dc_bit_offset = 0,
    .lcd_cmd_bits = 16,
    .lcd_param_bits = 0,
    .flags = {
        .dc_low_on_data = 0,
        .disable_control_phase = 1,
    },
    .scl_speed_hz = 400 * 1000,
};
static gpio_config_t PwrOff_GPIO = {
    .pin_bit_mask = (1ULL << GPIO_NUM_44),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

static struct tm CurrentTime;

/* RTC memory to persist across deep sleep */
RTC_DATA_ATTR static TimeManager_Config_t TimeManager_Config = {
    .Timezone = "",
    .NTPServer = "",
    .SyncInterval = 0,
    .BusHandle = NULL
};

RTC_DATA_ATTR static CalDAV_Config_t CalDAV_Config = {
    .ServerURL = "",
    .Username = "",
    .Password = "",
    .TimeoutMs = 0,
};

static const char *TAG = "main";

/** @brief          Flush function to copy the rendered content to the e-paper display.
 *                  This function is called by LVGL when a portion of the display needs to be updated.
 *                  Based on the FastEPD library LVGL example.
 *                  https://github.com/bitbank2/bb_lvgl/blob/main/FastEPD_touch_demo/FastEPD_touch_demo.ino
 *  @param disp
 *  @param area
 *  @param px_map
 */
static void epaper_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    static int iUpdates = 0; // count eink updates to know when to do a fullUpdate()
    static int iTop = 2000, iBottom = 0; // keep track of the range of rows to speed up partial updates
    uint8_t uc, count, *d; // we only operate in LVGL's A8 (8-bit grayscale) mode
    uint16_t u16, *s16;
    uint8_t u8Gray;

    // LVGL now supports a 1-bit graphics mode
    if (area->y1 < iTop) {
        iTop = area->y1;
    }

    if (area->y2 > iBottom) {
        iBottom = area->y2;
    }

    px_map += 8;

    if (epaper.getMode() == BB_MODE_1BPP) {
        for (uint32_t y = 0; y < lv_area_get_height(area); y++) {
            s16 = reinterpret_cast<uint16_t *>(&px_map[y * lv_area_get_width(area) * 2]);
            d = epaper.currentBuffer();
            d += ((y + area->y1) * ((epaper.width() + 7) / 8)) + (area->x1 >> 3);

            uint8_t bit_offset = area->x1 & 7;
            if (bit_offset != 0) {
                /* Start mid-byte: preserve existing bits */
                uc = *d & (0xFF << (8 - bit_offset));
                count = 8 - bit_offset;
            } else {
                /* Start at byte boundary */
                count = 8;
                uc = 0;
            }

            for (uint32_t x = 0; x < lv_area_get_width(area); x++) {
                uc <<= 1;
                u16 = *s16++;
                u8Gray = (u16 & 0x1f) + ((u16 & 0x7e0) >> 5) + (u16 >> 11);
                uc |= (u8Gray >= 64);
                count--;

                if (count == 0) {
                    *d++ = uc;
                    uc = 0;
                    count = 8;
                }
            }

            if (count < 8) {
                uc <<= count;
                *d = (*d & ((1 << count) - 1)) | uc;
            }
        }
    }
    /* 4-bpp */
    else {
        for (uint32_t y = 0; y < lv_area_get_height(area); y++) {
            d = epaper.currentBuffer();
            d += ((y + area->y1) * ((epaper.width() + 1) / 2)) + (area->x1 >> 1);
            s16 = reinterpret_cast<uint16_t *>(&px_map[y * lv_area_get_width(area) * 2]);

            for (uint32_t x = 0; x < lv_area_get_width(area); x += 2) { // work in pairs of pixels
                u16 = *s16++;
                u8Gray = (u16 & 0x1f) + ((u16 & 0x7e0) >> 5) + (u16 >> 11);
                u8Gray >>= 3; // turn it into a 4-bit grayscale value
                uc = u8Gray << 4; // left of the pair (top 4 bits)
                u16 = *s16++;
                u8Gray = (u16 & 0x1f) + ((u16 & 0x7e0) >> 5) + (u16 >> 11);
                uc |= (u8Gray >> 3);
                *d++ = uc;
            }
        }
    }

    ESP_LOGD(TAG, "Area update: %d,%d to %d,%d, w=%d, h=%d", area->x1, area->y1, area->x2, area->y2,
             lv_area_get_width(area), lv_area_get_height(area));

    lv_display_flush_ready(disp);
    if (lv_disp_flush_is_last(disp)) {
        ESP_LOGD(TAG, "Final flush - updating display (iTop=%d, iBottom=%d, updates=%d)", iTop, iBottom, iUpdates);

        if ((epaper.getMode() == BB_MODE_1BPP) && (iUpdates < 50)) {
            epaper.partialUpdate(true, iTop, iBottom);
        }
        /* 16-gray mode only supports full updates, or after 50 partial updates */
        else {
            epaper.fullUpdate(CLEAR_SLOW, false, NULL);
            iUpdates = 0;
        }

        /* Reset bounds AFTER the update */
        iTop = 2000;
        iBottom = 0;
        iUpdates++;
    }
}

static void on_Touch_Handler(lv_indev_t *p_Indev, lv_indev_data_t *p_Data)
{
    esp_lcd_touch_point_data_t Data[5];
    uint8_t Count;
    esp_err_t Error;

    Error = esp_lcd_touch_read_data(Touch_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_read_data failed: 0x%x", Error);

        p_Data->state = LV_INDEV_STATE_RELEASED;

        return;
    }

    Error = esp_lcd_touch_get_data(Touch_Handle, Data, &Count, sizeof(Data) / sizeof(Data[0]));
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_get_data failed: 0x%x", Error);

        p_Data->state = LV_INDEV_STATE_RELEASED;

        return;
    }

    if (Count > 0) {
        ESP_LOGD(TAG, "Touch at (%d, %d), strength=%d, points=%d", Data[0].x, Data[0].y, Data[0].strength, Count);

        p_Data->point.x = Data[0].y;
        p_Data->point.y = Data[0].x;
        p_Data->state = LV_INDEV_STATE_PRESSED;

        return;
    }

    p_Data->state = LV_INDEV_STATE_RELEASED;
}

extern "C" void app_main(void)
{
    esp_err_t Error;
    App_Settings_CalDAV_t CalDAV_Settings;
    App_Settings_WiFi_t WiFi_Settings;
    App_Settings_System_t System_Settings;
    Event_List_t *EventList;
    int BatteryVoltage;
    int8_t RSSI = 0;
    uint8_t BatteryPercentage;

    ESP_LOGD(TAG, "Reset reason: %d", esp_reset_reason());

    ESP_ERROR_CHECK(SettingsManager_Init());

    SettingsManager_GetWiFi(&WiFi_Settings);
    SettingsManager_GetSystem(&System_Settings);
    strcpy(reinterpret_cast<char *>(TimeManager_Config.Timezone), System_Settings.Timezone);
    strcpy(reinterpret_cast<char *>(TimeManager_Config.NTPServer), System_Settings.NTP_Server);
    TimeManager_Config.SyncInterval = System_Settings.NTP_SyncInterval;

    SettingsManager_GetCalDAV(&CalDAV_Settings);
    strcpy(reinterpret_cast<char *>(CalDAV_Config.ServerURL), CalDAV_Settings.URL);
    strcpy(reinterpret_cast<char *>(CalDAV_Config.Username), CalDAV_Settings.Username);
    strcpy(reinterpret_cast<char *>(CalDAV_Config.Password), CalDAV_Settings.Password);
    CalDAV_Config.TimeoutMs = CalDAV_Settings.TimeoutMs;

    I2CM_Init(&I2CM_Config, &I2C_Bus_Handle);
    TimeManager_Config.BusHandle = I2C_Bus_Handle;

    lv_init();

    epaper.initPanel(BB_PANEL_M5PAPERS3);
    epaper.setPanelSize(CONFIG_GUI_WIDTH, CONFIG_GUI_HEIGHT, BB_PANEL_FLAG_NONE);
    epaper.fillScreen(BBEP_WHITE);
    epaper.fullUpdate(CLEAR_SLOW, false, NULL);
    ESP_LOGD(TAG, "Display size: %dx%d", epaper.width(), epaper.height());

    ESP_LOGD(TAG, "Allocating %d bytes for draw buffer", GUI_DRAW_BUFFER_SIZE);
    LVGL_DrawBuffer = static_cast<uint16_t *>(heap_caps_malloc(GUI_DRAW_BUFFER_SIZE, MALLOC_CAP_SPIRAM));
    if (LVGL_DrawBuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate draw buffer!");

        return;
    }

    Display = lv_display_create(epaper.width(), epaper.height());
    if (Display == NULL) {
        ESP_LOGE(TAG, "Failed to create display!");

        heap_caps_free(LVGL_DrawBuffer);

        return;
    }

    lv_display_set_flush_cb(Display, epaper_display_flush);
    lv_display_set_buffers(Display, LVGL_DrawBuffer, NULL, GUI_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_rotation(Display, LV_DISPLAY_ROTATION_0);

    /* Get and clean the default screen to remove any artifacts */
    lv_obj_t *default_screen = lv_scr_act();
    lv_obj_remove_style_all(default_screen);
    lv_obj_clean(default_screen);
    lv_obj_clear_flag(default_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(default_screen, LV_SCROLLBAR_MODE_OFF);

    Screen_Status = UI_Status_Create();
    lv_scr_load(Screen_Status);
    lv_refr_now(Display);

    ADC_ReadBattery(&BatteryVoltage, &BatteryPercentage);
    UI_Status_Update_Battery(BatteryPercentage);

    /* Initialize TimeManager first to check RTC status and SNTP sync requirements */
    TimeManager_Init(&TimeManager_Config);

    bool NeedWiFi = false;
    bool NeedSNTPSync = false;

    /* Check if RTC time is valid */
    if (TimeManager_IsRTCValid() == false) {
        ESP_LOGW(TAG, "RTC time invalid (VL flag set) - SNTP sync required");
        NeedWiFi = true;
        NeedSNTPSync = true;
    } else {
        /* RTC is valid, sync system time from RTC */
        ESP_LOGI(TAG, "RTC time valid, syncing system time from RTC");
        TimeManager_SyncFromRTC();

        /* Check if periodic SNTP sync is due */
        if (TimeManager_IsSNTPSyncDue()) {
            ESP_LOGI(TAG, "Periodic SNTP sync is due");
            NeedWiFi = true;
            NeedSNTPSync = true;
        }
    }

    /* Always need WiFi for calendar data */
    NeedWiFi = true;

    /* Connect WiFi if needed */
    if (NeedWiFi) {
        ESP_LOGI(TAG, "Connecting to WiFi (SNTP sync: %s)...", NeedSNTPSync ? "yes" : "no");
        Error = WiFi_Init(WiFi_Settings.SSID, WiFi_Settings.Password, WiFi_Settings.MaxRetries, WiFi_Settings.TimeoutMs);
        if (Error != ESP_OK) {
            ESP_LOGE(TAG, "Failed to connect to WiFi: %d!", Error);

            WiFi_Deinit();

            /* Update status text */
            UI_Status_Update_Text(LV_SYMBOL_WARNING " No WiFi");
            lv_refr_now(Display);

            /* If SNTP sync was required but failed, continue with RTC time */
            if (NeedSNTPSync && TimeManager_IsRTCValid()) {
                ESP_LOGW(TAG, "WiFi failed but RTC is valid, continuing with RTC time");
            }

            goto main_power_down;
        }

        ESP_LOGI(TAG, "WiFi connected!");
        RSSI = WiFi_GetLastRSSI();
        UI_Status_Update_RSSI(RSSI);

        /* Initialize SNTP after WiFi is connected */
        Error = TimeManager_InitSNTP();
        if (Error != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SNTP: %d", Error);

            /* Skip SNTP sync if init failed */
            NeedSNTPSync = false;
        }

        /* Perform SNTP sync if needed */
        if (NeedSNTPSync) {
            ESP_LOGI(TAG, "Performing SNTP sync...");
            TimeManager_SyncFromSNTP(System_Settings.NTP_Timeout * 1000);
            if (TimeManager_GetTime(&CurrentTime) == ESP_OK) {
                ESP_LOGI(TAG, "Time updated via SNTP: %04d-%02d-%02d %02d:%02d:%02d",
                         CurrentTime.tm_year + 1900, CurrentTime.tm_mon + 1, CurrentTime.tm_mday,
                         CurrentTime.tm_hour, CurrentTime.tm_min, CurrentTime.tm_sec);
            }
        }
    } else {
        ESP_LOGI(TAG, "WiFi not needed, using RTC time");
    }

    TimeManager_GetTime(&CurrentTime);

    /* Fetch calendar events */
    if ((EventManager_Init() == ESP_OK) && (CalDAV_Client_Init(&CalDAV_Config, &Client) == ESP_OK)) {
        esp_err_t Error;
        time_t StartTime;
        time_t EndTime;

        struct tm Start = CurrentTime;
        Start.tm_hour = 0;
        Start.tm_min = 0;
        Start.tm_sec = 0;
        Start.tm_isdst = -1;

        struct tm End = CurrentTime;
        End.tm_hour = 23;
        End.tm_min = 59;
        End.tm_sec = 59;

#ifndef CONFIG_GUI_VIEW_LIST
        End.tm_mday += 2;
#endif

        /* Normalize time structures */
        StartTime = mktime(&Start);
        EndTime = mktime(&End);
        localtime_r(&StartTime, &Start);
        localtime_r(&EndTime, &End);

        ESP_LOGI(TAG, "Loading events from %04d-%02d-%02d %02d:%02d:%02d to %04d-%02d-%02d %02d:%02d:%02d",
                 Start.tm_year + 1900, Start.tm_mon + 1, Start.tm_mday,
                 Start.tm_hour, Start.tm_min, Start.tm_sec,
                 End.tm_year + 1900, End.tm_mon + 1, End.tm_mday,
                 End.tm_hour, End.tm_min, End.tm_sec);

        /* Load events using Event Manager */
        Error = EventManager_LoadEvents(&Client, &Start, &End);
        if (Error == ESP_OK) {
            ESP_LOGI(TAG, "Successfully loaded %d events", EventManager_GetEventCount());
        } else {
            ESP_LOGE(TAG, "Failed to load events: 0x%x!", Error);
        }

        CalDAV_Client_Deinit(&Client);
    } else {
        ESP_LOGE(TAG, "Failed to initialize CalDAV client!");
    }

    WiFi_Deinit();
    ESP_LOGD(TAG, "Battery: %d mV (%d%%)", BatteryVoltage, BatteryPercentage);

    Screen_Dashboard = GUI_VIEW_CREATE();
    lv_scr_load(Screen_Dashboard);

    /* Destroy the status screen after switching to free resources and
       prevent any leftover rendering artifacts on the e-paper display. */
    UI_Status_Destroy();

    GUI_VIEW_UPDATE_HEADER(&CurrentTime);
    GUI_VIEW_UPDATE_BATTERY(BatteryPercentage);
    GUI_VIEW_UPDATE_RSSI(RSSI);

    /* Add events from Event Manager to Dashboard */
    if ((EventManager_GetEvents(&EventList) == ESP_OK) && (EventList != NULL) && (EventList->Count > 0)) {
        Event_Node_t *p_Current = EventList->p_Head;
        int EventLimit = 0;

        /* Prepare current date for comparison (midnight) */
        struct tm CurrentDate = CurrentTime;
        CurrentDate.tm_hour = 0;
        CurrentDate.tm_min = 0;
        CurrentDate.tm_sec = 0;
        time_t CurrentDateTimestamp = mktime(&CurrentDate);

        ESP_LOGI(TAG, "Adding events to dashboard (filtering for current date: %04d-%02d-%02d)",
                 CurrentTime.tm_year + 1900, CurrentTime.tm_mon + 1, CurrentTime.tm_mday);

        while ((p_Current != NULL) && (EventLimit < UI_MAX_EVENTS)) {
            Event_t *p_Event = &p_Current->Event;
            struct tm StartTime;
            struct tm EndTime;
            char start_str[20];
            char end_str[20];
            bool show_event = false;

            localtime_r(&p_Event->StartTime, &StartTime);
            localtime_r(&p_Event->EndTime, &EndTime);

            /* Calculate day difference using timestamps for accurate cross-month/year comparison */
            struct tm EventDate = StartTime;
            EventDate.tm_hour = 0;
            EventDate.tm_min = 0;
            EventDate.tm_sec = 0;
            time_t EventDateTimestamp = mktime(&EventDate);
            int day_diff = static_cast<int>((EventDateTimestamp - CurrentDateTimestamp) / (24 * 60 * 60));

            /* Filter events based on type and date */
            if (p_Event->isAllDay) {
                /* All-day events: Show only on their actual date (day_diff == 0) */
                show_event = (day_diff == 0);
                ESP_LOGI(TAG, "All-day event '%s' on day_diff=%d -> %s",
                         p_Event->Title, day_diff, show_event ? "SHOW" : "SKIP");
            } else {
                /* Timed events: Show if they occur today or are currently ongoing
                 * (start before/during today and end after/during today)
                 */
                show_event = (day_diff == 0) || ((EventDateTimestamp <= CurrentDateTimestamp) &&
                                                 (p_Event->EndTime >= CurrentDateTimestamp));
                ESP_LOGI(TAG, "Timed event '%s' on day_diff=%d -> %s",
                         p_Event->Title, day_diff, show_event ? "SHOW" : "SKIP");
            }

            if (show_event) {
                snprintf(start_str, sizeof(start_str), "%02d:%02d", StartTime.tm_hour, StartTime.tm_min);
                snprintf(end_str, sizeof(end_str), "%02d:%02d", EndTime.tm_hour, EndTime.tm_min);

                ESP_LOGI(TAG, "Adding %s event: %s at %s-%s (date: %04d-%02d-%02d, day_diff=%d)",
                         p_Event->isAllDay ? "ALL-DAY" : "TIMED",
                         p_Event->Title, start_str, end_str,
                         StartTime.tm_year + 1900, StartTime.tm_mon + 1, StartTime.tm_mday,
                         day_diff);

                GUI_VIEW_ADD_EVENT(day_diff, StartTime.tm_hour,
                                   p_Event->Title,
                                   start_str,
                                   end_str,
                                   p_Event->Location,
                                   p_Event->Description);

                EventLimit++;
            }

            p_Current = p_Current->p_Next;
        }

        ESP_LOGD(TAG, "Added %d events to dashboard UI (filtered from %d total)", EventLimit, EventList->Count);

        if (EventLimit == 0) {
            ESP_LOGI(TAG, "No events for today after filtering");
            GUI_VIEW_CLEAR_EVENTS();
        }
    } else {
        ESP_LOGI(TAG, "No events to display");

        GUI_VIEW_CLEAR_EVENTS();
    }

    lv_refr_now(Display);

    EventManager_Deinit();

main_power_down:
    if (TimeManager_GetTime(&CurrentTime) == ESP_OK) {
        ESP_LOGI(TAG, "Current time: %02d:%02d:%02d", CurrentTime.tm_hour, CurrentTime.tm_min, CurrentTime.tm_sec);

        CurrentTime.tm_hour += System_Settings.SleepDurationHours;

        /* Normalize the time after adding the sleep time */
        mktime(&CurrentTime);

        /* Disable day/weekday filters for time-only alarm */
        CurrentTime.tm_mday = -1;
        CurrentTime.tm_wday = -1;

        ESP_LOGI(TAG, "Setting alarm for: %02d:%02d", CurrentTime.tm_hour, CurrentTime.tm_min);

        /* Set new alarm (overwrites any existing alarm) */
        TimeManager_SetAlarm(&CurrentTime);
    }

    TimeManager_Deinit();

    I2CM_Deinit(I2C_Bus_Handle);

    gpio_config(&PwrOff_GPIO);
    for (uint8_t i = 0; i < 5; ++i) {
        gpio_set_level(GPIO_NUM_44, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_44, 1);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}