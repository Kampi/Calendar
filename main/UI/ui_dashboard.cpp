/*
 * ui_dashboard.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: UI handling.
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
#include <esp_heap_caps.h>

#include <string.h>
#include <stdio.h>

#include "ui.h"

#define UI_HOUR_HEIGHT              55
#define UI_DAY_WIDTH                280
#define UI_TIME_LABEL_WIDTH         70
#define UI_HEADER_HEIGHT            70

/* Event data structure */
typedef struct {
    char *title;
    char *start_time;
    char *end_time;
    char *location;
    char *description;
} Event_Data_t;

static lv_obj_t *dashboard_screen = NULL;
static lv_obj_t *header_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *last_updated_label = NULL;
static lv_obj_t *calendar_container = NULL;
static lv_obj_t *day_labels[] = {NULL, NULL, NULL};
static lv_obj_t *info_modal = NULL;

static uint8_t _ui_Hours_Start;
static uint8_t _ui_Hours_End;

static const char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *months[] = {"January", "February", "March", "April", "May", "June",
                               "July", "August", "September", "October", "November", "December"
                              };

static const char *TAG = "UI";

/** @brief
 *  @param p_Event  Pointer to event data to display
 */
static void close_info_modal_cb(lv_event_t *p_Event)
{
    if (info_modal != NULL) {
        lv_obj_del(info_modal);
        info_modal = NULL;
    }
}

static void UI_Dashboard_Show_Event_Info(Event_Data_t *p_EventData)
{
    lv_obj_t *content;
    lv_obj_t *info_box;
    lv_obj_t *title_label;
    lv_obj_t *close_btn;
    lv_obj_t *close_label;

    if (info_modal != NULL) {
        lv_obj_del(info_modal);
        info_modal = NULL;
    }

    /* Create modal background */
    info_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(info_modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(info_modal, 0, 0);
    lv_obj_set_style_bg_color(info_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(info_modal, LV_OPA_50, 0);
    lv_obj_set_style_border_width(info_modal, 0, 0);
    lv_obj_clear_flag(info_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(info_modal, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(info_modal, close_info_modal_cb, LV_EVENT_CLICKED, NULL);

    /* Create info box */
    info_box = lv_obj_create(info_modal);
    lv_obj_set_size(info_box, LV_HOR_RES - 80, LV_VER_RES - 100);
    lv_obj_center(info_box);
    lv_obj_set_style_bg_color(info_box, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(info_box, 3, 0);
    lv_obj_set_style_border_color(info_box, lv_color_hex(0x404040), 0);
    lv_obj_set_style_radius(info_box, 10, 0);
    lv_obj_set_style_pad_all(info_box, 15, 0);
    lv_obj_clear_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(info_box, LV_SCROLLBAR_MODE_OFF);

    /* Create content container */
    content = lv_obj_create(info_box);
    lv_obj_set_size(content, lv_pct(100), lv_pct(85));
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 5, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    /* Title */
    title_label = lv_label_create(content);
    lv_label_set_text(title_label, p_EventData->title ? p_EventData->title : "Event");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x000000), 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title_label, lv_pct(100));
    lv_obj_set_style_pad_bottom(title_label, 10, 0);

    /* Time */
    if (p_EventData->start_time || p_EventData->end_time) {
        char time_str[100];
        lv_obj_t *time_container;
        lv_obj_t *time_icon;
        lv_obj_t *time_label;

        time_container = lv_obj_create(content);
        lv_obj_set_size(time_container, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(time_container, 1, 0);
        lv_obj_set_style_border_color(time_container, lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_radius(time_container, 5, 0);
        lv_obj_set_style_pad_all(time_container, 8, 0);

        time_icon = lv_label_create(time_container);
        // TODO
        //lv_label_set_text(time_icon, LV_SYMBOL_CLOCK);
        lv_obj_set_style_text_color(time_icon, lv_color_hex(0x404040), 0);
        lv_obj_align(time_icon, LV_ALIGN_TOP_LEFT, 0, 0);

        time_label = lv_label_create(time_container);
        if (p_EventData->start_time && p_EventData->end_time) {
            snprintf(time_str, sizeof(time_str), "%s - %s",
                     p_EventData->start_time, p_EventData->end_time);
        } else if (p_EventData->start_time) {
            snprintf(time_str, sizeof(time_str), "%s", p_EventData->start_time);
        } else {
            snprintf(time_str, sizeof(time_str), "%s", p_EventData->end_time);
        }

        lv_label_set_text(time_label, time_str);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0x000000), 0);
        lv_obj_align_to(time_label, time_icon, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    }

    /* Location */
    if (p_EventData->location && (strlen(p_EventData->location) > 0)) {
        lv_obj_t *loc_container;
        lv_obj_t *loc_icon;
        lv_obj_t *loc_label;

        loc_container = lv_obj_create(content);
        lv_obj_set_size(loc_container, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(loc_container, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_border_width(loc_container, 1, 0);
        lv_obj_set_style_border_color(loc_container, lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_radius(loc_container, 5, 0);
        lv_obj_set_style_pad_all(loc_container, 8, 0);

        loc_icon = lv_label_create(loc_container);
        lv_label_set_text(loc_icon, LV_SYMBOL_GPS);
        lv_obj_set_style_text_color(loc_icon, lv_color_hex(0x404040), 0);
        lv_obj_align(loc_icon, LV_ALIGN_TOP_LEFT, 0, 0);

        loc_label = lv_label_create(loc_container);
        lv_label_set_text(loc_label, p_EventData->location);
        lv_obj_set_style_text_font(loc_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(loc_label, lv_color_hex(0x000000), 0);
        lv_label_set_long_mode(loc_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(loc_label, lv_pct(85));
        lv_obj_align_to(loc_label, loc_icon, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    }

    /* Description */
    if (p_EventData->description && strlen(p_EventData->description) > 0) {
        lv_obj_t *desc_container;
        lv_obj_t *desc_icon;
        lv_obj_t *desc_label;

        desc_container = lv_obj_create(content);
        lv_obj_set_size(desc_container, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(desc_container, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_border_width(desc_container, 1, 0);
        lv_obj_set_style_border_color(desc_container, lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_radius(desc_container, 5, 0);
        lv_obj_set_style_pad_all(desc_container, 8, 0);

        desc_icon = lv_label_create(desc_container);
        lv_label_set_text(desc_icon, LV_SYMBOL_LIST);
        lv_obj_set_style_text_color(desc_icon, lv_color_hex(0x404040), 0);
        lv_obj_align(desc_icon, LV_ALIGN_TOP_LEFT, 0, 0);

        desc_label = lv_label_create(desc_container);
        lv_label_set_text(desc_label, p_EventData->description);
        lv_obj_set_style_text_font(desc_label, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(desc_label, lv_color_hex(0x000000), 0);
        lv_label_set_long_mode(desc_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(desc_label, lv_pct(85));
        lv_obj_align_to(desc_label, desc_icon, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    }

    /* Close button */
    close_btn = lv_btn_create(info_box);
    lv_obj_set_size(close_btn, lv_pct(100), 60);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xC0C0C0), 0);
    lv_obj_set_style_radius(close_btn, 5, 0);
    lv_obj_add_event_cb(close_btn, close_info_modal_cb, LV_EVENT_CLICKED, NULL);

    close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_22, 0);
    lv_obj_center(close_label);
}

/** @brief
 *  @param p_Event  Pointer to event data to display
 */
static void _ui_on_Clicked_Handler(lv_event_t *p_Event)
{
    Event_Data_t *event_data = (Event_Data_t *)lv_event_get_user_data(p_Event);
    if (event_data != NULL) {
        UI_Dashboard_Show_Event_Info(event_data);
    }
}

lv_obj_t* UI_Dashboard_Create(uint8_t Hours_Start, uint8_t Hours_End)
{
    lv_obj_t *header;
    lv_obj_t *time_line;

    _ui_Hours_Start = Hours_Start;
    _ui_Hours_End = Hours_End;

    /* Create main screen */
    dashboard_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(dashboard_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(dashboard_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dashboard_screen, LV_SCROLLBAR_MODE_OFF);

    /* Create header container */
    header = lv_obj_create(dashboard_screen);
    lv_obj_set_size(header, LV_HOR_RES, UI_HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_border_width(header, 2, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);

    /* Header title label */
    header_label = lv_label_create(header);
    lv_obj_set_style_text_font(header_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header_label, lv_color_hex(0x000000), 0);
    lv_obj_align(header_label, LV_ALIGN_LEFT_MID, 15, 0);

    /* Battery label */
    battery_label = lv_label_create(header);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL " 100%");
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x000000), 0);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* Last updated label (centered between week label and battery) */
    last_updated_label = lv_label_create(header);
    lv_label_set_text(last_updated_label, "Last updated: --:--:--");
    lv_obj_set_style_text_font(last_updated_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(last_updated_label, lv_color_hex(0x000000), 0);
    lv_obj_align(last_updated_label, LV_ALIGN_CENTER, 0, 0);

    /* Create calendar container */
    calendar_container = lv_obj_create(dashboard_screen);
    lv_obj_set_size(calendar_container, LV_HOR_RES - 20, LV_VER_RES - UI_HEADER_HEIGHT - 2);
    lv_obj_set_pos(calendar_container, 10, UI_HEADER_HEIGHT + 1);
    lv_obj_set_style_bg_color(calendar_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(calendar_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(calendar_container, 2, 0);
    lv_obj_set_style_border_color(calendar_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(calendar_container, 3, 0);
    lv_obj_set_style_pad_all(calendar_container, 5, 0);
    lv_obj_clear_flag(calendar_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(calendar_container, LV_SCROLLBAR_MODE_OFF);

    /* Create time column labels */
    for (uint8_t i = 0; i <= (_ui_Hours_End - _ui_Hours_Start); i++) {
        char time_str[10];
        lv_obj_t *time_label;

        time_label = lv_label_create(calendar_container);
        snprintf(time_str, sizeof(time_str), "%02d:00", _ui_Hours_Start + i);
        lv_label_set_text(time_label, time_str);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0x000000), 0);
        lv_obj_set_pos(time_label, 5, 43 + i * UI_HOUR_HEIGHT);
    }

    /* Create vertical line after time labels */
    time_line = lv_obj_create(calendar_container);
    lv_obj_set_size(time_line, 2, (_ui_Hours_End - _ui_Hours_Start + 1) * UI_HOUR_HEIGHT + 5);
    lv_obj_set_pos(time_line, UI_TIME_LABEL_WIDTH + 12, 50);
    lv_obj_set_style_bg_color(time_line, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(time_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(time_line, 0, 0);
    lv_obj_set_style_radius(time_line, 0, 0);

    /* Create day column headers */
    lv_obj_t *day_line;
    for (uint8_t i = 0; i < (sizeof(day_labels) / sizeof(day_labels[0])); i++) {
        day_labels[i] = lv_label_create(calendar_container);
        lv_label_set_text(day_labels[i], "Day");
        lv_obj_set_style_text_font(day_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(day_labels[i], lv_color_hex(0x000000), 0);
        lv_obj_set_pos(day_labels[i], UI_TIME_LABEL_WIDTH + 20 + i * UI_DAY_WIDTH, 5);

        /* Vertical separator between days */
        if (i < 2) {
            day_line = lv_obj_create(calendar_container);
            lv_obj_set_size(day_line, 2, (_ui_Hours_End - _ui_Hours_Start + 1) * UI_HOUR_HEIGHT + 5);
            lv_obj_set_pos(day_line, UI_TIME_LABEL_WIDTH + 12 + (i + 1) * UI_DAY_WIDTH, 50);
            lv_obj_set_style_bg_color(day_line, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(day_line, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(day_line, 0, 0);
            lv_obj_set_style_radius(day_line, 0, 0);
        }
    }
    day_line = lv_obj_create(calendar_container);
    lv_obj_set_size(day_line, 2, (_ui_Hours_End - _ui_Hours_Start + 1) * UI_HOUR_HEIGHT + 5);
    lv_obj_set_pos(day_line, UI_TIME_LABEL_WIDTH + 12 + (sizeof(day_labels) / sizeof(day_labels[0])) * UI_DAY_WIDTH, 50);
    lv_obj_set_style_bg_color(day_line, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(day_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(day_line, 0, 0);
    lv_obj_set_style_radius(day_line, 0, 0);

    /* Create horizontal lines for each hour */
    lv_obj_t *hour_line;
    for (uint8_t i = 0; i <= (_ui_Hours_End - _ui_Hours_Start); i++) {
        hour_line = lv_obj_create(calendar_container);
        lv_obj_set_size(hour_line, (sizeof(day_labels) / sizeof(day_labels[0])) * UI_DAY_WIDTH, 2);
        lv_obj_set_pos(hour_line, UI_TIME_LABEL_WIDTH + 12, 50 + i * UI_HOUR_HEIGHT);
        lv_obj_set_style_bg_color(hour_line, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(hour_line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(hour_line, 0, 0);
        lv_obj_set_style_radius(hour_line, 0, 0);
    }

    hour_line = lv_obj_create(calendar_container);
    lv_obj_set_size(hour_line, (sizeof(day_labels) / sizeof(day_labels[0])) * UI_DAY_WIDTH, 2);
    lv_obj_set_pos(hour_line, UI_TIME_LABEL_WIDTH + 12, 50 + 5 + (_ui_Hours_End - _ui_Hours_Start + 1) * UI_HOUR_HEIGHT);
    lv_obj_set_style_bg_color(hour_line, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(hour_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hour_line, 0, 0);
    lv_obj_set_style_radius(hour_line, 0, 0);

    return dashboard_screen;
}

void UI_Dashboard_Update(struct tm *p_TimeInfo)
{
    struct tm current_day ;
    char Text[50];

    if ((header_label == NULL) || (p_TimeInfo == NULL)) {
        return;
    }

    /* Get calendar week (ISO 8601) */
    strftime(Text, sizeof(Text), "Week %V", p_TimeInfo);

    lv_label_set_text(header_label, Text);

    /* Update day labels */
    current_day = *p_TimeInfo;
    for(uint8_t i = 0; i < (sizeof(day_labels) / sizeof(day_labels[0])); i++) {
        if (day_labels[i] != NULL) {
            char day_str[50];

            snprintf(day_str, sizeof(day_str), "%s, %s %02d",
                    weekdays[current_day.tm_wday],
                    months[current_day.tm_mon],
                    current_day.tm_mday);
            lv_label_set_text(day_labels[i], day_str);

            current_day.tm_mday++;
            mktime(&current_day);
        }
    }
}

void UI_Dashboard_Update_Battery(uint8_t Percentage)
{
    char Text[20];
    const char *Symbol;

    if (battery_label == NULL) {
        return;
    }

    if (Percentage > 80) {
        Symbol = LV_SYMBOL_BATTERY_FULL;
    } else if (Percentage > 60) {
        Symbol = LV_SYMBOL_BATTERY_3;
    } else if (Percentage > 40) {
        Symbol = LV_SYMBOL_BATTERY_2;
    } else if (Percentage > 20) {
        Symbol = LV_SYMBOL_BATTERY_1;
    } else {
        Symbol = LV_SYMBOL_BATTERY_EMPTY;
    }

    snprintf(Text, sizeof(Text), "%s %d%%", Symbol, Percentage);
    lv_label_set_text(battery_label, Text);
}

void UI_Dashboard_Update_LastUpdate(struct tm *p_TimeInfo)
{
    char Time[30];

    if ((last_updated_label == NULL) || (p_TimeInfo == NULL)) {
        return;
    }

    snprintf(Time, sizeof(Time), "Last updated: %02d:%02d:%02d",
             p_TimeInfo->tm_hour,
             p_TimeInfo->tm_min,
             p_TimeInfo->tm_sec);

    lv_label_set_text(last_updated_label, Time);
}

void UI_Dashboard_Add_Event(int DayDiff, uint8_t Hour, const char *Title,
                            const char *StartTime, const char *EndTime,
                            const char *Location, const char *Description)
{
    int day_col;
    int x_pos;
    int y_pos;
    int event_height;
    uint8_t start_hour;
    uint8_t start_min;
    uint8_t end_hour;
    uint8_t end_min;
    char TitleBuffer[64];
    int duration_minutes = 60;  /* Default: 1 hour */

    if ((calendar_container == NULL) || (Title == NULL)) {
        return;
    }

    day_col = DayDiff;
    if((DayDiff < 0) || (DayDiff > (sizeof(day_labels) / sizeof(day_labels[0]) - 1))) {
        day_col = 0;
    }

    /* Parse start and end times to get minutes */
    if (StartTime && (sscanf(StartTime, "%hhu:%hhu", &start_hour, &start_min) == 2)) {
        int hours_offset;
        float minute_fraction;

        /* Calculate precise Y position based on hour and minutes */
        if ((start_hour < _ui_Hours_Start) || (start_hour > _ui_Hours_End)) {
            ESP_LOGW(TAG, "Hour %d out of range (%d-%d)", start_hour, _ui_Hours_Start, _ui_Hours_End);
            return;
        }

        /* Calculate Y position */
        hours_offset = start_hour - _ui_Hours_Start;
        minute_fraction = start_min / 60.0f;
        y_pos = 50 + (hours_offset * UI_HOUR_HEIGHT) + (int)(minute_fraction * UI_HOUR_HEIGHT) + 2;
    } else {
        /* Fallback to hour-based positioning */
        if ((Hour < _ui_Hours_Start) || (Hour > _ui_Hours_End)) {
            ESP_LOGW(TAG, "Hour %d out of range (%d-%d)", Hour, _ui_Hours_Start, _ui_Hours_End);
            return;
        }

        y_pos = 50 + (Hour - _ui_Hours_Start) * UI_HOUR_HEIGHT + 2;
        start_hour = Hour;
        start_min = 0;
    }

    /* Calculate event height based on duration */
    if (EndTime && (sscanf(EndTime, "%hhu:%hhu", &end_hour, &end_min) == 2)) {
        duration_minutes = (end_hour * 60 + end_min) - (start_hour * 60 + start_min);
        if (duration_minutes <= 0) {
            duration_minutes = 60;  /* Default to 1 hour */
        }

        /* Convert duration to pixels (UI_HOUR_HEIGHT pixels per 60 minutes) */
        event_height = (duration_minutes * UI_HOUR_HEIGHT) / 60 - 2;

        /* Minimum height for readability */
        if (event_height < 18) {
            event_height = 18;
        }
    } else {
        /* Default height: 1 hour */
        event_height = UI_HOUR_HEIGHT;
    }

    /* Center event box in cell with equal padding on both sides */
    int desired_padding = 10;  /* Equal padding on left and right */
    int cell_start = UI_TIME_LABEL_WIDTH + 14 + day_col * UI_DAY_WIDTH;  /* Right edge of left line */
    x_pos = cell_start + desired_padding;

    /* Allocate and store event data */
    Event_Data_t *event_data = (Event_Data_t *)heap_caps_malloc(sizeof(Event_Data_t), MALLOC_CAP_SPIRAM);
    if (event_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate event data!");
        return;
    }

    event_data->title = Title ? strdup(Title) : NULL;
    event_data->start_time = StartTime ? strdup(StartTime) : NULL;
    event_data->end_time = EndTime ? strdup(EndTime) : NULL;
    event_data->location = Location ? strdup(Location) : NULL;
    event_data->description = Description ? strdup(Description) : NULL;

    /* Create event box */
    lv_obj_t *event_box = lv_obj_create(calendar_container);
    lv_obj_set_size(event_box, UI_DAY_WIDTH - 20, event_height);
    lv_obj_set_pos(event_box, x_pos, y_pos);

    lv_obj_set_style_border_width(event_box, 3, 0);
    lv_obj_set_style_border_color(event_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_opa(event_box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(event_box, 0, 0);
    lv_obj_set_style_pad_left(event_box, 8, 0);
    lv_obj_set_style_pad_right(event_box, 5, 0);
    lv_obj_set_style_pad_top(event_box, 3, 0);
    lv_obj_set_style_pad_bottom(event_box, 3, 0);
    lv_obj_add_flag(event_box, LV_OBJ_FLAG_CLICKABLE);

    /* Add click event with event data */
    lv_obj_add_event_cb(event_box, _ui_on_Clicked_Handler, LV_EVENT_CLICKED, event_data);

    /* Create event label */
    lv_obj_t *event_label = lv_label_create(event_box);
    snprintf(TitleBuffer, sizeof(TitleBuffer), "%s - %02hhu:%02hhu (%d minutes)", Title, start_hour, start_min, duration_minutes);
    lv_label_set_text(event_label, TitleBuffer);
    lv_obj_set_style_text_font(event_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(event_label, lv_color_hex(0x000000), 0);
    lv_label_set_long_mode(event_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(event_label, UI_DAY_WIDTH - 35);
    lv_obj_align(event_label, LV_ALIGN_TOP_LEFT, 0, 0);

    ESP_LOGI(TAG, "Added event '%s' at pos(%d,%d) height=%d duration=%d min, start=%02hhu:%02hhu end=%02hhu:%02hhu", 
             Title, x_pos, y_pos, event_height, duration_minutes, start_hour, start_min, end_hour, end_min);
}

void UI_Dashboard_Clear_Events(void)
{
    if (calendar_container == NULL) {
        return;
    }

    /* Remove all event boxes and free event data */
    uint32_t child_cnt = lv_obj_get_child_cnt(calendar_container);
    for (int32_t i = (child_cnt - 1); i >= 0; i--) {
        lv_obj_t *child = lv_obj_get_child(calendar_container, i);
        /* Check if it's an event box by checking if it has a label child */
        if (lv_obj_get_child_cnt(child) > 0) {
            lv_obj_t *first_child = lv_obj_get_child(child, 0);
            if (lv_obj_check_type(first_child, &lv_label_class)) {
                /* Verify it's not one of our day labels or time labels */
                if ((first_child != day_labels[0]) && (first_child != day_labels[1])) {
                    /* Free event data if attached */
                    Event_Data_t *event_data = (Event_Data_t *)lv_obj_get_user_data(child);
                    if (event_data != NULL) {
                        if (event_data->title) {
                            free(event_data->title);
                        }

                        if (event_data->start_time) {
                            free(event_data->start_time);
                        }

                        if (event_data->end_time) {
                            free(event_data->end_time);
                        }

                        if (event_data->location) {
                            free(event_data->location);
                        }

                        if (event_data->description) {
                            free(event_data->description);
                        }

                        heap_caps_free(event_data);
                    }

                    lv_obj_del(child);
                }
            }
        }
    }

    ESP_LOGI(TAG, "Cleared all events and freed memory");
}

void UI_Dashboard_Destroy(void)
{
    if (dashboard_screen != NULL) {
        lv_obj_del(dashboard_screen);

        dashboard_screen = NULL;
        header_label = NULL;
        battery_label = NULL;
    }
}