/*
 * eventTypes.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Event data structures and types for calendar event management.
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

#pragma once

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum length for event title.
 */
#define EVENT_TITLE_MAX_LENGTH      64

/** @brief Maximum length for event description.
 */
#define EVENT_DESCRIPTION_MAX_LENGTH 256

/** @brief Maximum length for event location.
 */
#define EVENT_LOCATION_MAX_LENGTH   64

/** @brief Maximum length for event UID.
 */
#define EVENT_UID_MAX_LENGTH        128

/** @brief Timeslot duration in minutes (30 minutes).
 */
#define EVENT_TIMESLOT_MINUTES      30

/** @brief Number of timeslots per hour (2 slots of 30 minutes each).
 */
#define EVENT_TIMESLOTS_PER_HOUR    2

/** @brief Number of timeslots per day (48 slots for 24 hours).
 */
#define EVENT_TIMESLOTS_PER_DAY     48

/** @brief Calendar event structure.
 */
typedef struct {
    char UID[EVENT_UID_MAX_LENGTH];                     /**< Unique event identifier. */
    char Title[EVENT_TITLE_MAX_LENGTH];                 /**< Event title/summary. */
    char Description[EVENT_DESCRIPTION_MAX_LENGTH];     /**< Event description. */
    char Location[EVENT_LOCATION_MAX_LENGTH];           /**< Event location. */
    char Calendar[64];                                  /**< Calendar name. */
    time_t StartTime;                                   /**< Event start time (Unix timestamp). */
    time_t EndTime;                                     /**< Event end time (Unix timestamp). */
    bool isAllDay;                                      /**< True if event is all-day. */
} Event_t;

/** @brief Timeslot index structure for grid positioning.
 */
typedef struct {
    uint8_t Day;                /**< Day index (0 = today, 1 = tomorrow, 2 = day after). */
    uint8_t Hour;               /**< Hour of day (0-23). */
    uint8_t HalfHour;           /**< Half-hour flag (0 = :00, 1 = :30). */
    uint8_t AbsoluteSlot;       /**< Absolute slot index (0-47 for 24 hours). */
} TimeSlot_Index_t;

/** @brief Event list node for linked list.
 */
typedef struct Event_Node {
    Event_t Event;              /**< Event data. */
    struct Event_Node *p_Next;  /**< Pointer to next event in list. */
} Event_Node_t;

/** @brief Event list structure.
 */
typedef struct {
    Event_Node_t *p_Head;       /**< Head of event list. */
    size_t Count;               /**< Number of events in list. */
} Event_List_t;

#ifdef __cplusplus
}
#endif
