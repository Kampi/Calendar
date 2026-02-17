/*
 * eventManager.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Event manager implementation for loading and managing calendar events.
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
#include <list>
#include <string>

#include "eventManager.h"
#include "../Settings/settingsManager.h"

static const char *TAG = "EventManager";

/** @brief Event manager state structure.
 */
typedef struct {
    Event_List_t Events;            /**< List of all loaded events. */
    bool isInitialized;             /**< Initialization flag. */
    SemaphoreHandle_t Mutex;        /**< Mutex for thread-safe access. */
} EventManager_State_t;

static EventManager_State_t _State = {
    .Events = {
        .p_Head = NULL,
        .Count = 0
    },
    .isInitialized = false,
    .Mutex = NULL
};

/** @brief          Check if iCalendar date/time string is a DATE (all-day event).
 *  @param p_DateTimeStr  iCalendar date/time string
 *  @return         true if DATE format (YYYYMMDD), false if DATETIME format
 */
static bool is_icalendar_date_format(const char *p_DateTimeStr)
{
    if (p_DateTimeStr == NULL) {
        return false;
    }

    /* Check for DATE format: YYYYMMDD (8 characters)
     * DATETIME format contains 'T': YYYYMMDDTHHMMSSZ (at least 15 characters)
     * Also check for VALUE=DATE parameter in the string
     */
    const char *p_ValueDate = strstr(p_DateTimeStr, "VALUE=DATE");
    if (p_ValueDate != NULL) {
        return true;
    }

    /* Look for the actual date value after colons */
    const char *p_Value = strrchr(p_DateTimeStr, ':');
    if (p_Value != NULL) {
        p_Value++;  /* Skip the ':' */
    } else {
        p_Value = p_DateTimeStr;
    }

    /* Check if there's no 'T' separator (DATE format) */
    return (strchr(p_Value, 'T') == NULL) && (strlen(p_Value) == 8);
}

/** @brief          Portable replacement for timegm(): converts a UTC struct tm to a Unix timestamp.
 *                  mktime() treats the struct as local time, so the UTC offset is added back.
 *  @param p_Tm     Pointer to struct tm with UTC values
 *  @return         Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
 */
static time_t portable_timegm(struct tm *p_Tm)
{
    time_t t = mktime(p_Tm);
    struct tm utc;
    gmtime_r(&t, &utc);
    utc.tm_isdst = 0;
    time_t t_utc = mktime(&utc);

    return t + (t - t_utc);
}

/** @brief          Parse iCalendar datetime string to Unix timestamp.
 *  @param p_DateTimeStr iCalendar datetime string (e.g., "20260125T100000Z")
 *  @return         Unix timestamp, or 0 on error
 */
static time_t parse_icalendar_datetime(const char *p_DateTimeStr)
{
    struct tm tm_time;
    const char *p_Value;

    if (p_DateTimeStr == NULL) {
        return 0;
    }

    /* Extract value after last colon (handles "DTSTART:20260201" or "DTSTART;VALUE=DATE:20260201") */
    p_Value = strrchr(p_DateTimeStr, ':');
    if (p_Value != NULL) {
        p_Value++;  /* Skip the ':' */
    } else {
        p_Value = p_DateTimeStr;
    }

    memset(&tm_time, 0, sizeof(struct tm));

    /* Try parsing DATETIME format first: YYYYMMDDTHHMMSSZ */
    if (sscanf(p_Value, "%4d%2d%2dT%2d%2d%2d",
               &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
               &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec) == 6) {
        tm_time.tm_year -= 1900;  /* Years since 1900 */
        tm_time.tm_mon -= 1;      /* Months since January (0-11) */

        /* Check for UTC 'Z' suffix - use timegm() so the values are treated as
         * UTC instead of local time, avoiding a timezone-offset error.
         * Events stored with TZID (no 'Z') are already in local time → mktime(). */
        if (strchr(p_Value, 'Z') != NULL) {
            tm_time.tm_isdst = 0;
            return portable_timegm(&tm_time);
        }

        tm_time.tm_isdst = -1;    /* Let mktime determine DST */

        return mktime(&tm_time);
    }

    /* Try parsing DATE format: YYYYMMDD (all-day events) */
    if (sscanf(p_Value, "%4d%2d%2d",
               &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday) == 3) {
        tm_time.tm_year -= 1900;  /* Years since 1900 */
        tm_time.tm_mon -= 1;      /* Months since January (0-11) */
        tm_time.tm_hour = 0;      /* Midnight */
        tm_time.tm_min = 0;
        tm_time.tm_sec = 0;
        tm_time.tm_isdst = -1;    /* Let mktime determine DST */

        return mktime(&tm_time);
    }

    return 0;
}

/** @brief          Parse iCalendar DURATION string (e.g. "PT1H30M") to seconds.
 *                  Supports weeks (W), days (D), hours (H), minutes (M) and seconds (S).
 *  @param p_DurationStr iCalendar DURATION value string
 *  @return         Duration in seconds, or 0 on error
 */
static time_t parse_icalendar_duration(const char *p_DurationStr)
{
    const char *p;
    int64_t TotalSeconds = 0;
    int64_t n = 0;
    bool InTime = false;

    if (p_DurationStr == NULL) {
        return 0;
    }

    p = p_DurationStr;

    /* Skip optional sign and 'P' designator */
    if ((*p == '+') || (*p == '-')) {
        p++;
    }

    if (*p == 'P') {
        p++;
    }

    while (*p) {
        if (*p == 'T') {
            InTime = true;
            p++;
            continue;
        }

        if ((*p >= '0') && (*p <= '9')) {
            n = n * 10 + (*p - '0');
        } else if (*p == 'W') {
            TotalSeconds += n * 7 * 24 * 3600;
            n = 0;
        } else if (*p == 'D') {
            TotalSeconds += n * 24 * 3600;
            n = 0;
        } else if ((*p == 'H') && InTime) {
            TotalSeconds += n * 3600;
            n = 0;
        } else if ((*p == 'M') && InTime) {
            TotalSeconds += n * 60;
            n = 0;
        } else if ((*p == 'S') && InTime) {
            TotalSeconds += n;
            n = 0;
        }

        p++;
    }

    return static_cast<time_t>(TotalSeconds);
}

/** @brief          Copy UTF-8 string into destination buffer (bounded, UTF-8-safe).
 *                  Passes all characters through as-is so LVGL can render umlauts
 *                  directly using the extended font glyphs.
 *  @param p_Dest   Destination buffer
 *  @param p_Src    Source string with UTF-8 characters
 *  @param MaxLen   Maximum length of destination buffer
 */
static void convert_german_chars(char *p_Dest, const char *p_Src, size_t MaxLen)
{
    size_t DestIdx = 0;
    const unsigned char *p_Src_u = (const unsigned char *)p_Src;

    if ((p_Dest == NULL) || (p_Src == NULL) || (MaxLen == 0)) {
        return;
    }

    while (*p_Src_u && (DestIdx < MaxLen - 1)) {
        /* Determine byte length of current UTF-8 sequence */
        size_t SeqLen;

        if (*p_Src_u < 0x80) {
            SeqLen = 1;
        } else if ((*p_Src_u & 0xE0) == 0xC0) {
            SeqLen = 2;
        } else if ((*p_Src_u & 0xF0) == 0xE0) {
            SeqLen = 3;
        } else if ((*p_Src_u & 0xF8) == 0xF0) {
            SeqLen = 4;
        } else {
            /* Invalid UTF-8 start byte, skip */
            p_Src_u++;
            continue;
        }

        /* Only copy if the complete sequence fits in the buffer */
        if ((DestIdx + SeqLen) < MaxLen) {
            for (size_t j = 0; j < SeqLen; j++) {
                if (p_Src_u[j] == 0) {
                    break;  /* Premature end of string */
                }
                p_Dest[DestIdx++] = static_cast<char>(p_Src_u[j]);
            }
        }

        p_Src_u += SeqLen;  /* Advance past the UTF-8 sequence */
    }

    p_Dest[DestIdx] = '\0';
}

/** @brief          Add event to internal list.
 *  @param p_Event  Event to add
 *  @return         ESP_OK on success, error code otherwise
 */
static esp_err_t add_event_to_list(Event_t *p_Event)
{
    Event_Node_t *p_Node;

    if (p_Event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    p_Node = static_cast<Event_Node_t *>(malloc(sizeof(Event_Node_t)));
    if (p_Node == NULL) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(&p_Node->Event, p_Event, sizeof(Event_t));
    p_Node->p_Next = _State.Events.p_Head;
    _State.Events.p_Head = p_Node;
    _State.Events.Count++;

    return ESP_OK;
}

esp_err_t EventManager_Init(void)
{
    if (_State.isInitialized) {
        ESP_LOGW(TAG, "Event Manager already initialized");

        return ESP_OK;
    }

    _State.Mutex = xSemaphoreCreateMutex();
    if (_State.Mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex!");

        return ESP_FAIL;
    }

    _State.Events.p_Head = NULL;
    _State.Events.Count = 0;
    _State.isInitialized = true;

    ESP_LOGD(TAG, "Event Manager initialized");

    return ESP_OK;
}

void EventManager_Deinit(void)
{
    if (_State.isInitialized == false) {
        return;
    }

    xSemaphoreTake(_State.Mutex, portMAX_DELAY);

    Event_Node_t *p_Current = _State.Events.p_Head;
    while (p_Current != NULL) {
        Event_Node_t *p_Next = p_Current->p_Next;
        free(p_Current);
        p_Current = p_Next;
    }

    _State.Events.p_Head = NULL;
    _State.Events.Count = 0;

    xSemaphoreGive(_State.Mutex);
    vSemaphoreDelete(_State.Mutex);

    _State.Mutex = NULL;
    _State.isInitialized = false;

    ESP_LOGD(TAG, "Event Manager deinitialized");
}

esp_err_t EventManager_LoadEvents(CalDAV_Client_t *p_Client, struct tm *p_Start, struct tm *p_End)
{
    CalDAV_Calendar_List_t Calendars;
    CalDAV_Error_t Error_CalDAV;
    std::list<std::string> CalendarList;

    if (_State.isInitialized == false) {
        ESP_LOGE(TAG, "Event Manager not initialized");
        return ESP_ERR_INVALID_STATE;
    } else if ((p_Client == NULL) || (p_Start == NULL) || (p_End == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(_State.Mutex, portMAX_DELAY);

    /* Clear existing events */
    Event_Node_t *p_Current = _State.Events.p_Head;
    while (p_Current != NULL) {
        Event_Node_t *p_Next = p_Current->p_Next;
        free(p_Current);
        p_Current = p_Next;
    }
    _State.Events.p_Head = NULL;
    _State.Events.Count = 0;

    xSemaphoreGive(_State.Mutex);

    /* Get configured calendar names */
    SettingsManager_GetCalendars(&CalendarList);
    ESP_LOGD(TAG, "Loading events from %d configured calendars", CalendarList.size());

    /* Get list of available calendars from server */
    Error_CalDAV = CalDAV_Calendars_List(p_Client, &Calendars);
    if (Error_CalDAV != CALDAV_ERROR_OK) {
        ESP_LOGE(TAG, "Failed to retrieve calendar list (Error: %d)", Error_CalDAV);

        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Found %d calendars on server", Calendars.Length);

    if (Calendars.Length == 0) {
        ESP_LOGW(TAG, "No calendars found on server");

        CalDAV_Calendars_Free(&Calendars);

        return ESP_OK;
    }

    /* Convert time range to UTC */
    time_t start_time_t = mktime(p_Start);
    time_t end_time_t = mktime(p_End);

    /* Use gmtime_r for thread-safe conversion to separate structures */
    struct tm start_utc = {};
    struct tm end_utc = {};
    gmtime_r(&start_time_t, &start_utc);
    gmtime_r(&end_time_t, &end_utc);

    ESP_LOGD(TAG, "Time range UTC: %04d-%02d-%02dT%02d:%02d:%02d to %04d-%02d-%02dT%02d:%02d:%02d",
             start_utc.tm_year + 1900, start_utc.tm_mon + 1, start_utc.tm_mday,
             start_utc.tm_hour, start_utc.tm_min, start_utc.tm_sec,
             end_utc.tm_year + 1900, end_utc.tm_mon + 1, end_utc.tm_mday,
             end_utc.tm_hour, end_utc.tm_min, end_utc.tm_sec);

    /* Iterate through configured calendars */
    std::list<std::string>::iterator It;
    for (It = CalendarList.begin(); It != CalendarList.end(); ++It) {
        CalDAV_Calendar_t *p_Calendar = NULL;
        const char *p_CalendarName = (*It).c_str();

        ESP_LOGD(TAG, "Looking for calendar: %s", p_CalendarName);

        /* Find calendar by name */
        Error_CalDAV = CalDAV_Calendar_Find_By_Name(&Calendars, p_CalendarName, &p_Calendar);
        if (Error_CalDAV != CALDAV_ERROR_OK) {
            ESP_LOGW(TAG, "Calendar '%s' not found on server", p_CalendarName);
            continue;
        }

        ESP_LOGD(TAG, "Found calendar '%s' at path: %s", p_CalendarName, p_Calendar->Path);

        /* Load events from this calendar */
        CalDAV_Calendar_Event_t *p_Events = NULL;
        size_t EventCount = 0;

        Error_CalDAV = CalDAV_Calendar_Events_List(p_Client, &p_Events, &EventCount,
                                                   p_Calendar->Path, &start_utc, &end_utc);
        if (Error_CalDAV != CALDAV_ERROR_OK) {
            ESP_LOGE(TAG, "Failed to load events from calendar '%s' (Error: %d)",
                     p_CalendarName, Error_CalDAV);
            continue;
        }

        ESP_LOGD(TAG, "Loaded %d events from calendar '%s'", EventCount, p_CalendarName);

        /* Process each event */
        for (size_t i = 0; i < EventCount; i++) {
            Event_t Event = {};

            /* Copy event data */
            if (p_Events[i].UID) {
                strncpy(Event.UID, p_Events[i].UID, EVENT_UID_MAX_LENGTH - 1);
            }

            if (p_Events[i].Summary) {
                convert_german_chars(Event.Title, p_Events[i].Summary, EVENT_TITLE_MAX_LENGTH);
            }

            if (p_Events[i].Description) {
                convert_german_chars(Event.Description, p_Events[i].Description, EVENT_DESCRIPTION_MAX_LENGTH);
            }

            if (p_Events[i].Location) {
                convert_german_chars(Event.Location, p_Events[i].Location, EVENT_LOCATION_MAX_LENGTH);
            }

            strncpy(Event.Calendar, p_CalendarName, sizeof(Event.Calendar) - 1);

            /* Parse times */
            ESP_LOGD(TAG, "Parsing event '%s': StartTime='%s', EndTime='%s', Duration='%s'",
                     Event.Title,
                     p_Events[i].StartTime ? p_Events[i].StartTime : "NULL",
                     p_Events[i].EndTime ? p_Events[i].EndTime : "NULL",
                     p_Events[i].Duration ? p_Events[i].Duration : "NULL");
            Event.StartTime = parse_icalendar_datetime(p_Events[i].StartTime);
            Event.EndTime = parse_icalendar_datetime(p_Events[i].EndTime);

            /* Fallback: compute end time from DURATION when DTEND is absent (RFC 5545 allows either) */
            if ((Event.EndTime == 0) && (Event.StartTime != 0) && (p_Events[i].Duration != NULL)) {
                time_t Duration = parse_icalendar_duration(p_Events[i].Duration);
                if (Duration > 0) {
                    Event.EndTime = Event.StartTime + Duration;
                    ESP_LOGD(TAG, "Event '%s': computed EndTime from DURATION '%s' (%ld s)",
                             Event.Title, p_Events[i].Duration, (long)Duration);
                }
            }

            /* Detect all-day events by checking if DTSTART uses DATE format */
            Event.isAllDay = is_icalendar_date_format(p_Events[i].StartTime);

            ESP_LOGD(TAG, "Event '%s' detected as %s (StartTime format check)",
                     Event.Title, Event.isAllDay ? "ALL-DAY" : "TIMED");

            /* Add to internal list */
            xSemaphoreTake(_State.Mutex, portMAX_DELAY);
            esp_err_t Error = add_event_to_list(&Event);
            xSemaphoreGive(_State.Mutex);

            if (Error != ESP_OK) {
                ESP_LOGE(TAG, "Failed to add event to list: %s", Event.Title);
            } else {
                ESP_LOGD(TAG, "Added event: %s (Start: %ld, End: %ld)",
                         Event.Title, Event.StartTime, Event.EndTime);
            }
        }

        CalDAV_Events_Free(p_Events, EventCount);
    }

    CalDAV_Calendars_Free(&Calendars);

    ESP_LOGD(TAG, "Total events loaded: %d", _State.Events.Count);

    return ESP_OK;
}

esp_err_t EventManager_GetEvents(Event_List_t **pp_List)
{
    if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    } else if (pp_List == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(_State.Mutex, portMAX_DELAY);
    *pp_List = &_State.Events;
    xSemaphoreGive(_State.Mutex);

    return ESP_OK;
}

esp_err_t EventManager_GetEventsForSlot(TimeSlot_Index_t *p_Slot, Event_List_t **pp_List)
{
    Event_List_t *p_FilteredList;
    struct tm SlotStartTime = {};
    time_t SlotStartT;
    time_t SlotEndT;

    if (_State.isInitialized == false) {
        return ESP_ERR_INVALID_STATE;
    } else if ((p_Slot == NULL) || (pp_List == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Convert slot to time range */
    EventManager_SlotToTime(p_Slot, &SlotStartTime);
    SlotStartT = mktime(&SlotStartTime);

    /* End time is 30 minutes after start */
    SlotEndT = SlotStartT + (EVENT_TIMESLOT_MINUTES * 60);

    /* Create filtered list */
    p_FilteredList = static_cast<Event_List_t *>(malloc(sizeof(Event_List_t)));
    if (p_FilteredList == NULL) {
        return ESP_ERR_NO_MEM;
    }

    p_FilteredList->p_Head = NULL;
    p_FilteredList->Count = 0;

    xSemaphoreTake(_State.Mutex, portMAX_DELAY);

    /* Iterate through all events and filter by time */
    Event_Node_t *p_Current = _State.Events.p_Head;
    while (p_Current != NULL) {
        Event_t *p_Event = &p_Current->Event;

        /* Check if event overlaps with this timeslot */
        if ((p_Event->StartTime < SlotEndT) && (p_Event->EndTime > SlotStartT)) {
            Event_Node_t *p_NewNode = static_cast<Event_Node_t *>(malloc(sizeof(Event_Node_t)));
            if (p_NewNode != NULL) {
                memcpy(&p_NewNode->Event, p_Event, sizeof(Event_t));
                p_NewNode->p_Next = p_FilteredList->p_Head;
                p_FilteredList->p_Head = p_NewNode;
                p_FilteredList->Count++;
            }
        }

        p_Current = p_Current->p_Next;
    }

    xSemaphoreGive(_State.Mutex);

    *pp_List = p_FilteredList;

    return ESP_OK;
}

esp_err_t EventManager_TimeToSlot(struct tm *p_Time, TimeSlot_Index_t *p_Slot)
{
    if ((p_Time == NULL) || (p_Slot == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    p_Slot->Hour = p_Time->tm_hour;
    p_Slot->HalfHour = (p_Time->tm_min >= 30) ? 1 : 0;
    p_Slot->AbsoluteSlot = (p_Slot->Hour * EVENT_TIMESLOTS_PER_HOUR) + p_Slot->HalfHour;
    p_Slot->Day = 0;  /* Relative day index (needs to be calculated elsewhere) */

    return ESP_OK;
}

esp_err_t EventManager_SlotToTime(TimeSlot_Index_t *p_Slot, struct tm *p_Time)
{
    if ((p_Slot == NULL) || (p_Time == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(p_Time, 0, sizeof(struct tm));

    p_Time->tm_hour = p_Slot->Hour;
    p_Time->tm_min = p_Slot->HalfHour * 30;
    p_Time->tm_sec = 0;

    return ESP_OK;
}

void EventManager_FreeEventList(Event_List_t *p_List)
{
    if (p_List == NULL) {
        return;
    }

    Event_Node_t *p_Current = p_List->p_Head;
    while (p_Current != NULL) {
        Event_Node_t *p_Next = p_Current->p_Next;
        free(p_Current);
        p_Current = p_Next;
    }

    free(p_List);
}

size_t EventManager_GetEventCount(void)
{
    if (_State.isInitialized == false) {
        return 0;
    }

    return _State.Events.Count;
}
