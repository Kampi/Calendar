/*
 * ui_dayview.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Day view UI for CalDAV events.
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

#include <string.h>
#include <stdio.h>

#include "ui.h"

#define UI_HEADER_HEIGHT            70
#define UI_EVENT_BOX_HEIGHT         120
#define UI_EVENT_BOX_MARGIN         10

static lv_obj_t *dayview_screen = NULL;
static lv_obj_t *header_label = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *event_list = NULL;
static lv_obj_t *no_events_label = NULL;
static lv_obj_t *rssi_label = NULL;

lv_obj_t* UI_DayView_Create(void)
{
    lv_obj_t *header;

    /* Create main screen */
    dayview_screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(dayview_screen);

    /* Set fullscreen white background */
    lv_obj_set_size(dayview_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(dayview_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(dayview_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(dayview_screen, 0, 0);

    /* Create header container */
    header = lv_obj_create(dayview_screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_HOR_RES, UI_HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_outline_width(header, 0, 0);
    lv_obj_set_style_shadow_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 10, 0);

    /* Header title label */
    header_label = lv_label_create(header);
    lv_obj_set_style_text_font(header_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(header_label, lv_color_hex(0x000000), 0);
    lv_obj_align(header_label, LV_ALIGN_LEFT_MID, 5, 0);

    /* Battery label */
    battery_label = lv_label_create(header);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL " 100%");
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x000000), 0);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -10, 0);

    /* RSSI label */
    rssi_label = lv_label_create(header);
    lv_label_set_text(rssi_label, "");
    lv_obj_set_style_text_font(rssi_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(rssi_label, lv_color_hex(0x000000), 0);
    lv_obj_align(rssi_label, LV_ALIGN_CENTER, 200, 0);

    /* Create event list container */
    event_list = lv_obj_create(dayview_screen);
    lv_obj_remove_style_all(event_list);
    lv_obj_set_size(event_list, LV_HOR_RES - 20, LV_VER_RES - UI_HEADER_HEIGHT - 20);
    lv_obj_set_pos(event_list, 10, UI_HEADER_HEIGHT + 10);
    lv_obj_set_style_bg_color(event_list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(event_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(event_list, 0, 0);
    lv_obj_set_style_outline_width(event_list, 0, 0);
    lv_obj_set_style_shadow_width(event_list, 0, 0);
    lv_obj_set_style_pad_all(event_list, 0, 0);
    lv_obj_set_style_pad_row(event_list, UI_EVENT_BOX_MARGIN, 0);
    lv_obj_set_flex_flow(event_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(event_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    return dayview_screen;
}

void UI_DayView_Update_Header(struct tm *p_TimeInfo)
{
    char Buffer[64];

    if ((header_label != NULL) && (p_TimeInfo != NULL)) {
        const char *Weekdays[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
        const char *Months[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", 
                                "Juli", "August", "September", "Oktober", "November", "Dezember"};

        snprintf(Buffer, sizeof(Buffer), "%s, %d. %s %d",
                 Weekdays[p_TimeInfo->tm_wday],
                 p_TimeInfo->tm_mday,
                 Months[p_TimeInfo->tm_mon],
                 p_TimeInfo->tm_year + 1900);

        lv_label_set_text(header_label, Buffer);
    }
}

void UI_DayView_Update_Battery(uint8_t Percentage)
{
    char Buffer[16];

    if (battery_label != NULL) {
        if (Percentage > 80) {
            snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_BATTERY_FULL " %d%%", Percentage);
        } else if (Percentage > 60) {
            snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_BATTERY_3 " %d%%", Percentage);
        } else if (Percentage > 40) {
            snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_BATTERY_2 " %d%%", Percentage);
        } else if (Percentage > 20) {
            snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_BATTERY_1 " %d%%", Percentage);
        } else {
            snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_BATTERY_EMPTY " %d%%", Percentage);
        }

        lv_label_set_text(battery_label, Buffer);
    }
}

void UI_DayView_Update_RSSI(int8_t RSSI)
{
    char Buffer[16];

    if (rssi_label != NULL) {
        snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_WIFI " %d dBm", RSSI);
        lv_label_set_text(rssi_label, Buffer);
    }
}

void UI_DayView_Add_Event(int DayDiff, uint8_t Hour, const char *Title,
                          const char *StartTime, const char *EndTime,
                          const char *Location, const char *Description)
{
    char TimeBuffer[32];

    if (event_list == NULL) {
        return;
    }

    /* Hide "no events" label when adding events */
    if (no_events_label != NULL) {
        lv_obj_add_flag(no_events_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Create event box */
    lv_obj_t *event_box = lv_obj_create(event_list);
    lv_obj_set_size(event_box, LV_PCT(98), UI_EVENT_BOX_HEIGHT);
    lv_obj_set_style_bg_color(event_box, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(event_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(event_box, 3, 0);
    lv_obj_set_style_border_color(event_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(event_box, 8, 0);
    lv_obj_set_style_pad_all(event_box, 10, 0);

    /* Left side: Time container */
    lv_obj_t *time_container = lv_obj_create(event_box);
    lv_obj_set_size(time_container, 120, LV_PCT(100));
    lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_container, 0, 0);
    lv_obj_set_style_pad_all(time_container, 0, 0);
    lv_obj_align(time_container, LV_ALIGN_LEFT_MID, 0, 0);

    /* Time label */
    lv_obj_t *time_label = lv_label_create(time_container);
    snprintf(TimeBuffer, sizeof(TimeBuffer), "%s\n-\n%s", 
             StartTime ? StartTime : "--:--",
             EndTime ? EndTime : "--:--");
    lv_label_set_text(time_label, TimeBuffer);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

    /* Vertical separator line */
    lv_obj_t *separator = lv_obj_create(event_box);
    lv_obj_set_size(separator, 2, LV_PCT(80));
    lv_obj_set_style_bg_color(separator, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(separator, 0, 0);
    lv_obj_set_style_radius(separator, 0, 0);
    lv_obj_align(separator, LV_ALIGN_LEFT_MID, 125, 0);

    /* Right side: Event details container */
    lv_obj_t *details_container = lv_obj_create(event_box);
    lv_obj_set_size(details_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(details_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(details_container, 0, 0);
    lv_obj_set_style_pad_left(details_container, 140, 0);
    lv_obj_set_style_pad_top(details_container, 0, 0);
    lv_obj_set_style_pad_right(details_container, 0, 0);
    lv_obj_align(details_container, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Title label */
    lv_obj_t *title_label = lv_label_create(details_container);
    if (Title != NULL) {
        lv_label_set_text(title_label, Title);
        lv_label_set_recolor(title_label, true);
    } else {
        lv_label_set_text(title_label, "No title");
    }
    lv_obj_set_width(title_label, LV_PCT(100));
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x000000), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Location label */
    lv_obj_t *location_label = lv_label_create(details_container);
    if ((Location != NULL) && (strlen(Location) > 0)) {
        char LocBuffer[256];
        snprintf(LocBuffer, sizeof(LocBuffer), LV_SYMBOL_GPS " %s", Location);
        lv_label_set_text(location_label, LocBuffer);
        lv_label_set_recolor(location_label, true);
    } else {
        lv_label_set_text(location_label, "");
    }
    lv_obj_set_width(location_label, LV_PCT(100));
    lv_label_set_long_mode(location_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(location_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(location_label, lv_color_hex(0x000000), 0);
    lv_obj_set_pos(location_label, 0, 28);

    /* Description label */
    lv_obj_t *desc_label = lv_label_create(details_container);
    if ((Description != NULL) && (strlen(Description) > 0)) {
        lv_label_set_text(desc_label, Description);
        lv_label_set_recolor(desc_label, true);
    } else {
        lv_label_set_text(desc_label, "");
    }
    lv_obj_set_width(desc_label, LV_PCT(100));
    lv_label_set_long_mode(desc_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(desc_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(desc_label, lv_color_hex(0x000000), 0);
    lv_obj_align(desc_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

void UI_DayView_Clear_Events(void)
{
    if (event_list != NULL) {
        lv_obj_clean(event_list);

        /* Re-create "no events" label */
        no_events_label = lv_label_create(dayview_screen);
        lv_label_set_text(no_events_label, "No events today " LV_SYMBOL_OK);
        lv_obj_set_style_text_font(no_events_label, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(no_events_label, lv_color_hex(0x000000), 0);
        lv_obj_clear_flag(no_events_label, LV_OBJ_FLAG_HIDDEN);

        /* Center the label in the event list */
        lv_obj_align(no_events_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_align(no_events_label, LV_TEXT_ALIGN_CENTER, 0);
    }
}

void UI_DayView_Show_No_Events(void)
{
    if (no_events_label != NULL) {
        lv_obj_clear_flag(no_events_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void UI_DayView_Destroy(void)
{
    if (dayview_screen != NULL) {
        lv_obj_del(dayview_screen);

        dayview_screen = NULL;
    }
}
