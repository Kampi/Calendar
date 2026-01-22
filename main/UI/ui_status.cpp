/*
 * ui_loading.cpp
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

#include <string.h>
#include <stdio.h>

#include "ui.h"

#define UI_HEADER_HEIGHT            70

static lv_obj_t* status_label;
static lv_obj_t* battery_label;
static lv_obj_t* header;
static lv_obj_t *status_screen;
static lv_obj_t *rssi_label;

lv_obj_t* UI_Status_Create(void)
{
    /* Create main screen */
    status_screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(status_screen);

    /* Set fullscreen white background */
    lv_obj_set_size(status_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(status_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(status_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(status_screen, 0, 0);

    /* Create header container */
    header = lv_obj_create(status_screen);
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

    /* Create status label */
    status_label = lv_label_create(status_screen);
    lv_label_set_text(status_label, LV_SYMBOL_DOWNLOAD " Loading");
    lv_obj_set_width(status_label, LV_PCT(100));
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

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

    return status_screen;
}

void UI_Status_Update_Text(const char* p_Text)
{
    if ((status_label != NULL) && (p_Text != NULL)) {
        lv_label_set_text(status_label, p_Text);
    }
}

void UI_Status_Update_Battery(uint8_t Percentage)
{
    char Buffer[16];

    if (battery_label != NULL) {
        /* Create battery string with symbol */
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

        /* Only update the value if it has changed */
        if (strcmp(lv_label_get_text(battery_label), Buffer) != 0) {
            lv_label_set_text(battery_label, Buffer);
        }
    }
}

void UI_Status_Update_RSSI(int8_t RSSI)
{
    char Buffer[16];

    if (rssi_label != NULL) {
        snprintf(Buffer, sizeof(Buffer), LV_SYMBOL_WIFI " %d dBm", RSSI);

        /* Only update the value if it has changed */
        if (strcmp(lv_label_get_text(rssi_label), Buffer) != 0) {
            lv_label_set_text(rssi_label, Buffer);
        }
    }
}

void UI_Status_Destroy(void)
{
    if (status_screen != NULL) {
        lv_obj_del(status_screen);
        status_screen = NULL;
    }
}