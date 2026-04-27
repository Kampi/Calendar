/*
 * eventView.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Event view UI component implementation for displaying calendar events.
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
#include <string.h>
#include <stdlib.h>

#include "eventView.h"
#include "../EventManager/eventManager.h"

static const char *TAG = "event_view";

/** @brief Minimum height for an event cell in pixels.
 */
#define EVENT_CELL_MIN_HEIGHT   30

/** @brief Padding between event cells in pixels.
 */
#define EVENT_CELL_PADDING      2

/** @brief Event view handle structure.
 */
struct EventView_Handle {
    lv_obj_t *p_Container;                  /**< Main container object. */
    lv_obj_t *p_TimeGrid;                   /**< Time grid container. */
    lv_obj_t *p_EventContainer;             /**< Event container. */
    EventView_Config_t Config;              /**< View configuration. */
    EventView_ClickCallback_t ClickCallback;/**< Click callback function. */
    void *p_UserData;                       /**< User data for callback. */
    uint16_t SlotHeight;                    /**< Height of each 30-minute slot in pixels. */
    uint8_t VisibleSlotCount;               /**< Number of visible 30-minute slots. */
};

/** @brief Event button user data structure.
 */
typedef struct {
    Event_t Event;                          /**< Event data. */
    EventView_Handle_t *p_ViewHandle;       /**< Parent view handle. */
} EventButton_UserData_t;

/** @brief      Event button click handler.
 *  @param e    LVGL event structure
 */
static void event_button_clicked(lv_event_t *e)
{
    EventButton_UserData_t *p_UserData = static_cast<EventButton_UserData_t *>(lv_event_get_user_data(e));

    if ((p_UserData == NULL) || (p_UserData->p_ViewHandle == NULL)) {
        return;
    }

    EventView_Handle_t *p_Handle = p_UserData->p_ViewHandle;

    /* Call user callback if set */
    if (p_Handle->ClickCallback != NULL) {
        p_Handle->ClickCallback(&p_UserData->Event, p_Handle->p_UserData);
    } else {
        /* Default: Show event details popup */
        EventView_ShowEventDetails(&p_UserData->Event);
    }
}

/** @brief  Free user_data when the button is deleted by lv_obj_clean / lv_obj_del.
 */
static void event_button_deleted(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

/** @brief          Calculate slot height based on configuration.
 *  @param p_Config View configuration
 *  @return         Slot height in pixels
 */
static uint16_t calculate_slot_height(EventView_Config_t *p_Config)
{
    uint8_t hours_displayed = p_Config->EndHour - p_Config->StartHour;
    uint8_t slots_displayed = hours_displayed * EVENT_TIMESLOTS_PER_HOUR;

    return p_Config->Height / slots_displayed;
}

/** @brief          Create time axis labels.
 *  @param p_Handle View handle
 */
static void create_time_axis(EventView_Handle_t *p_Handle)
{
    lv_obj_t *p_TimeAxis;
    char time_label[8];

    p_TimeAxis = lv_obj_create(p_Handle->p_Container);
    lv_obj_set_size(p_TimeAxis, 50, p_Handle->Config.Height);
    lv_obj_set_pos(p_TimeAxis, 0, 0);
    lv_obj_clear_flag(p_TimeAxis, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(p_TimeAxis, 0, 0);
    lv_obj_set_style_pad_all(p_TimeAxis, 0, 0);

    /* Create labels for each hour */
    for (uint8_t hour = p_Handle->Config.StartHour; hour <= p_Handle->Config.EndHour; hour++) {
        lv_obj_t *p_Label = lv_label_create(p_TimeAxis);

        snprintf(time_label, sizeof(time_label), "%02d:00", hour);
        lv_label_set_text(p_Label, time_label);

        uint16_t y_pos = (hour - p_Handle->Config.StartHour) * p_Handle->SlotHeight * EVENT_TIMESLOTS_PER_HOUR;
        lv_obj_set_pos(p_Label, 5, y_pos);
        lv_obj_set_style_text_font(p_Label, &lv_font_montserrat_ext_12, 0);
    }
}

/** @brief          Create day column headers.
 *  @param p_Handle View handle
 */
static void create_day_headers(EventView_Handle_t *p_Handle)
{
    const char *day_names[] = {"Heute", "Morgen", "Übermorgen"};
    uint16_t col_width = (p_Handle->Config.Width - 50) / p_Handle->Config.DayCount;

    for (uint8_t day = 0; day < p_Handle->Config.DayCount; day++) {
        lv_obj_t *p_Header = lv_label_create(p_Handle->p_Container);

        if (day < 3) {
            lv_label_set_text(p_Header, day_names[day]);
        } else {
            lv_label_set_text(p_Header, "");
        }

        lv_obj_set_pos(p_Header, 50 + (day * col_width) + (col_width / 2) - 30, -25);
        lv_obj_set_style_text_font(p_Header, &lv_font_montserrat_ext_14, 0);
    }
}

/** @brief          Create grid lines for timeslots.
 *  @param p_Handle View handle
 */
static void create_grid_lines(EventView_Handle_t *p_Handle)
{
    p_Handle->p_TimeGrid = lv_obj_create(p_Handle->p_Container);
    lv_obj_set_size(p_Handle->p_TimeGrid, p_Handle->Config.Width - 50, p_Handle->Config.Height);
    lv_obj_set_pos(p_Handle->p_TimeGrid, 50, 0);
    lv_obj_clear_flag(p_Handle->p_TimeGrid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(p_Handle->p_TimeGrid, 1, 0);
    lv_obj_set_style_pad_all(p_Handle->p_TimeGrid, 0, 0);
    lv_obj_set_style_bg_opa(p_Handle->p_TimeGrid, LV_OPA_0, 0);

    /* Draw horizontal lines for each 30-minute slot */
    for (uint8_t i = 0; i <= p_Handle->VisibleSlotCount; i++) {
        lv_obj_t *p_Line = lv_obj_create(p_Handle->p_TimeGrid);
        lv_obj_set_size(p_Line, p_Handle->Config.Width - 50, 1);
        lv_obj_set_pos(p_Line, 0, i * p_Handle->SlotHeight);
        lv_obj_set_style_bg_color(p_Line, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(p_Line, 0, 0);
    }

    /* Draw vertical lines for day columns */
    uint16_t col_width = (p_Handle->Config.Width - 50) / p_Handle->Config.DayCount;
    for (uint8_t day = 0; day <= p_Handle->Config.DayCount; day++) {
        lv_obj_t *p_Line = lv_obj_create(p_Handle->p_TimeGrid);
        lv_obj_set_size(p_Line, 1, p_Handle->Config.Height);
        lv_obj_set_pos(p_Line, day * col_width, 0);
        lv_obj_set_style_bg_color(p_Line, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(p_Line, 0, 0);
    }
}

/** @brief          Calculate event position in view.
 *  @param p_Handle View handle
 *  @param p_Event  Event to position
 *  @param p_X      Pointer to receive X position
 *  @param p_Y      Pointer to receive Y position
 *  @param p_Width  Pointer to receive width
 *  @param p_Height Pointer to receive height
 *  @return         ESP_OK on success, error code otherwise
 */
static esp_err_t calculate_event_position(EventView_Handle_t *p_Handle, Event_t *p_Event,
                                          int16_t *p_X, int16_t *p_Y,
                                          uint16_t *p_Width, uint16_t *p_Height)
{
    struct tm StartTime;
    struct tm EndTime;

    if ((localtime_r(&p_Event->StartTime, &StartTime) == NULL) ||
        (localtime_r(&p_Event->EndTime, &EndTime) == NULL)) {
        return ESP_FAIL;
    }

    /* Calculate day column */
    uint8_t day = 0;  /* TODO: Calculate relative day from current day */
    uint16_t col_width = (p_Handle->Config.Width - 50) / p_Handle->Config.DayCount;

    /* Calculate vertical position based on start time */
    TimeSlot_Index_t start_slot;
    EventManager_TimeToSlot(&StartTime, &start_slot);

    if ((start_slot.Hour < p_Handle->Config.StartHour) || (start_slot.Hour >= p_Handle->Config.EndHour)) {
        return ESP_ERR_INVALID_ARG;  /* Event outside visible hours */
    }

    uint8_t slot_offset = start_slot.Hour - p_Handle->Config.StartHour;
    *p_Y = (slot_offset * EVENT_TIMESLOTS_PER_HOUR + start_slot.HalfHour) * p_Handle->SlotHeight;

    /* Calculate height based on event duration */
    int64_t duration_seconds = p_Event->EndTime - p_Event->StartTime;
    int64_t duration_minutes = duration_seconds / 60;
    uint16_t duration_slots = (duration_minutes + EVENT_TIMESLOT_MINUTES - 1) / EVENT_TIMESLOT_MINUTES;

    *p_Height = duration_slots * p_Handle->SlotHeight - EVENT_CELL_PADDING * 2;
    if (*p_Height < EVENT_CELL_MIN_HEIGHT) {
        *p_Height = EVENT_CELL_MIN_HEIGHT;
    }

    /* Calculate horizontal position */
    *p_X = day * col_width + EVENT_CELL_PADDING;
    *p_Width = col_width - EVENT_CELL_PADDING * 2;

    return ESP_OK;
}

esp_err_t EventView_Create(EventView_Config_t *p_Config, EventView_Handle_t **pp_Handle)
{
    EventView_Handle_t *p_Handle;

    if ((p_Config == NULL) || (pp_Handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    } else if (p_Config->p_Parent == NULL) {
        ESP_LOGE(TAG, "Parent object is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    p_Handle = static_cast<EventView_Handle_t *>(malloc(sizeof(EventView_Handle_t)));
    if (p_Handle == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memset(p_Handle, 0, sizeof(EventView_Handle_t));
    memcpy(&p_Handle->Config, p_Config, sizeof(EventView_Config_t));

    /* Calculate slot height */
    p_Handle->SlotHeight = calculate_slot_height(p_Config);
    p_Handle->VisibleSlotCount = (p_Config->EndHour - p_Config->StartHour) * EVENT_TIMESLOTS_PER_HOUR;

    /* Create main container */
    p_Handle->p_Container = lv_obj_create(p_Config->p_Parent);
    lv_obj_set_size(p_Handle->p_Container, p_Config->Width, p_Config->Height);
    lv_obj_set_style_pad_all(p_Handle->p_Container, 0, 0);
    lv_obj_clear_flag(p_Handle->p_Container, LV_OBJ_FLAG_SCROLLABLE);

    /* Create event container */
    p_Handle->p_EventContainer = lv_obj_create(p_Handle->p_Container);
    lv_obj_set_size(p_Handle->p_EventContainer, p_Config->Width - 50, p_Config->Height);
    lv_obj_set_pos(p_Handle->p_EventContainer, 50, 0);
    lv_obj_set_style_bg_opa(p_Handle->p_EventContainer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(p_Handle->p_EventContainer, 0, 0);
    lv_obj_set_style_pad_all(p_Handle->p_EventContainer, 0, 0);

    /* Create UI elements */
    create_time_axis(p_Handle);
    create_day_headers(p_Handle);
    create_grid_lines(p_Handle);

    *pp_Handle = p_Handle;

    ESP_LOGI(TAG, "Event view created (Slot height: %d px, Visible slots: %d)",
             p_Handle->SlotHeight, p_Handle->VisibleSlotCount);

    return ESP_OK;
}

void EventView_Destroy(EventView_Handle_t *p_Handle)
{
    if (p_Handle == NULL) {
        return;
    }

    if (p_Handle->p_Container != NULL) {
        lv_obj_del(p_Handle->p_Container);
    }

    free(p_Handle);
}

esp_err_t EventView_Update(EventView_Handle_t *p_Handle)
{
    Event_List_t *p_EventList;
    esp_err_t Error;

    if (p_Handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Clear existing event objects */
    lv_obj_clean(p_Handle->p_EventContainer);

    /* Get all events */
    Error = EventManager_GetEvents(&p_EventList);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get events: 0x%x", Error);
        return Error;
    }

    if (p_EventList == NULL || p_EventList->Count == 0) {
        ESP_LOGD(TAG, "No events to display");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Displaying %d events", p_EventList->Count);

    /* Iterate through events and create UI elements */
    Event_Node_t *p_Current = p_EventList->p_Head;
    while (p_Current != NULL) {
        Event_t *p_Event = &p_Current->Event;
        int16_t x, y;
        uint16_t width, height;

        /* Calculate position */
        Error = calculate_event_position(p_Handle, p_Event, &x, &y, &width, &height);
        if (Error == ESP_OK) {
            /* Create event button */
            lv_obj_t *p_EventBtn = lv_btn_create(p_Handle->p_EventContainer);
            lv_obj_set_pos(p_EventBtn, x, y);
            lv_obj_set_size(p_EventBtn, width, height);
            lv_obj_set_style_bg_color(p_EventBtn, lv_color_hex(0x3399FF), 0);
            lv_obj_set_style_radius(p_EventBtn, 5, 0);

            /* Create label for event title */
            lv_obj_t *p_Label = lv_label_create(p_EventBtn);
            lv_label_set_text(p_Label, p_Event->Title);
            lv_label_set_long_mode(p_Label, LV_LABEL_LONG_DOT);
            lv_obj_set_width(p_Label, width - 10);
            lv_obj_center(p_Label);
            lv_obj_set_style_text_font(p_Label, &lv_font_montserrat_ext_12, 0);
            lv_obj_set_style_text_color(p_Label, lv_color_hex(0xFFFFFF), 0);

            /* Attach event data and click handler */
            EventButton_UserData_t *p_UserData = static_cast<EventButton_UserData_t *>(malloc(sizeof(EventButton_UserData_t)));
            if (p_UserData != NULL) {
                memcpy(&p_UserData->Event, p_Event, sizeof(Event_t));
                p_UserData->p_ViewHandle = p_Handle;
                lv_obj_add_event_cb(p_EventBtn, event_button_clicked, LV_EVENT_CLICKED, p_UserData);
                lv_obj_add_event_cb(p_EventBtn, event_button_deleted, LV_EVENT_DELETE, p_UserData);
            }

            ESP_LOGD(TAG, "Created event button: %s at (%d, %d) size (%d x %d)",
                     p_Event->Title, x, y, width, height);
        } else {
            ESP_LOGD(TAG, "Event outside visible range: %s", p_Event->Title);
        }

        p_Current = p_Current->p_Next;
    }

    return ESP_OK;
}

esp_err_t EventView_SetClickCallback(EventView_Handle_t *p_Handle,
                                     EventView_ClickCallback_t Callback,
                                     void *p_UserData)
{
    if (p_Handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    p_Handle->ClickCallback = Callback;
    p_Handle->p_UserData = p_UserData;

    return ESP_OK;
}

esp_err_t EventView_ShowEventDetails(Event_t *p_Event)
{
    lv_obj_t *p_Popup;
    lv_obj_t *p_Label;
    char buffer[512];
    struct tm StartTime;
    struct tm EndTime;

    if (p_Event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Create popup */
    p_Popup = lv_msgbox_create(NULL);
    lv_msgbox_add_title(p_Popup, "Termin Details");

    /* Use localtime_r to fill separate structs – localtime() shares a static
     * buffer and the second call would overwrite the first result. */
    localtime_r(&p_Event->StartTime, &StartTime);
    localtime_r(&p_Event->EndTime, &EndTime);

    snprintf(buffer, sizeof(buffer),
             "Titel: %s\n\n"
             "Kalender: %s\n\n"
             "Start: %02d:%02d\n"
             "Ende: %02d:%02d\n\n"
             "%s%s%s",
             p_Event->Title,
             p_Event->Calendar,
             StartTime.tm_hour, StartTime.tm_min,
             EndTime.tm_hour, EndTime.tm_min,
             p_Event->Location[0] ? "Ort: " : "",
             p_Event->Location,
             p_Event->Description[0] ? "\n\nBeschreibung:\n" : "");

    if (p_Event->Description[0]) {
        strncat(buffer, p_Event->Description, sizeof(buffer) - strlen(buffer) - 1);
    }

    p_Label = lv_msgbox_add_text(p_Popup, buffer);
    lv_obj_set_style_text_font(p_Label, &lv_font_montserrat_ext_12, 0);

    /* Add close button */
    lv_msgbox_add_close_button(p_Popup);

    ESP_LOGI(TAG, "Showing details for event: %s", p_Event->Title);

    return ESP_OK;
}
