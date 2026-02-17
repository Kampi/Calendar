/*
 * eventView.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Event view UI component for displaying calendar events in timeslots.
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

#ifndef EVENT_VIEW_H_
#define EVENT_VIEW_H_

#include <esp_err.h>

#include <lvgl.h>
#include "../Export/fonts/custom_fonts.h"

#include "../EventManager/eventTypes.h"

/** @brief Event view configuration.
 */
typedef struct {
    lv_obj_t *p_Parent;             /**< Parent LVGL object for the event view. */
    uint16_t Width;                 /**< View width in pixels. */
    uint16_t Height;                /**< View height in pixels. */
    uint8_t StartHour;              /**< Start hour for display (e.g., 6 for 6:00 AM). */
    uint8_t EndHour;                /**< End hour for display (e.g., 22 for 10:00 PM). */
    uint8_t DayCount;               /**< Number of days to display (1-3). */
} EventView_Config_t;

/** @brief Event view handle.
 */
typedef struct EventView_Handle EventView_Handle_t;

/** @brief Event click callback function type.
 *  @param p_Event  Pointer to the clicked event
 *  @param p_UserData User data passed during initialization
 */
typedef void (*EventView_ClickCallback_t)(Event_t *p_Event, void *p_UserData);

/** @brief          Create and initialize an event view.
 *  @param p_Config View configuration
 *  @param pp_Handle Pointer to receive view handle
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventView_Create(EventView_Config_t *p_Config, EventView_Handle_t **pp_Handle);

/** @brief          Destroy an event view and free resources.
 *  @param p_Handle View handle
 */
void EventView_Destroy(EventView_Handle_t *p_Handle);

/** @brief          Update the event view with current events.
 *  @param p_Handle View handle
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventView_Update(EventView_Handle_t *p_Handle);

/** @brief          Set event click callback.
 *  @param p_Handle View handle
 *  @param Callback Click callback function
 *  @param p_UserData User data to pass to callback
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventView_SetClickCallback(EventView_Handle_t *p_Handle,
                                     EventView_ClickCallback_t Callback,
                                     void *p_UserData);

/** @brief          Show event details popup.
 *  @param p_Event  Event to show details for
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventView_ShowEventDetails(Event_t *p_Event);

#endif /* EVENT_VIEW_H_ */