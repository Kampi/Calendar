/*
 * eventManager.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Event manager for loading and managing calendar events.
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

#ifndef EVENT_MANAGER_H_
#define EVENT_MANAGER_H_

#include <esp_err.h>

#include <caldav_client.h>

#include "eventTypes.h"

/** @brief  Initialize the Event Manager.
 *  @return ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_Init(void);

/** @brief Deinitialize the Event Manager and free resources.
 */
void EventManager_Deinit(void);

/** @brief          Load events from configured calendars for a time range.
 *  @param p_Client CalDAV client handle
 *  @param p_Start  Start time (inclusive)
 *  @param p_End    End time (inclusive)
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_LoadEvents(CalDAV_Client_t *p_Client, struct tm *p_Start, struct tm *p_End);

/** @brief          Get all loaded events.
 *  @param pp_List  Pointer to receive event list pointer
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_GetEvents(Event_List_t **pp_List);

/** @brief          Get events for a specific timeslot.
 *  @param p_Slot   Timeslot index
 *  @param pp_List  Pointer to receive filtered event list
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_GetEventsForSlot(TimeSlot_Index_t *p_Slot, Event_List_t **pp_List);

/** @brief          Convert time to timeslot index.
 *  @param p_Time   Time structure
 *  @param p_Slot   Pointer to receive timeslot index
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_TimeToSlot(struct tm *p_Time, TimeSlot_Index_t *p_Slot);

/** @brief          Convert timeslot index to time.
 *  @param p_Slot   Timeslot index
 *  @param p_Time   Pointer to receive time structure
 *  @return         ESP_OK on success, error code otherwise
 */
esp_err_t EventManager_SlotToTime(TimeSlot_Index_t *p_Slot, struct tm *p_Time);

/** @brief          Free an event list.
 *  @param p_List   Event list to free
 */
void EventManager_FreeEventList(Event_List_t *p_List);

/** @brief  Get event count.
 *  @return Number of loaded events
 */
size_t EventManager_GetEventCount(void);

#endif /* EVENT_MANAGER_H_ */