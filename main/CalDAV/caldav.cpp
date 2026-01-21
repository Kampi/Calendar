#include <esp_log.h>

#include <stdio.h>

#include "caldav.h"

static CalDAV_Config_t _CalDAV_Config = {
    .ServerURL = "https://admin.kampert.synology.me/caldav.php/Daniel%20Kampert",
    .Username = "Daniel Kampert",
    .Password = "do3PQKmk",
    .TimeoutMs = CONFIG_CALENDAR_CALDAV_SERVER_TIMEOUT_MS,
};

static const char *TAG = "CalDAV";

CalDAV_Client_t *CalDAV_Init(void)
{
    ESP_LOGI(TAG, "Initialize CalDAV interface...");

    CalDAV_Client_t *p_Client = CalDAV_Client_Init(&_CalDAV_Config);
    if (p_Client == NULL) {
        ESP_LOGE(TAG, "CalDAV client initialization failed!");
    }

    return p_Client;
}

esp_err_t CalDAV_Deinit(CalDAV_Client_t *p_Client)
{
    if(p_Client == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Deinitialize CalDAV interface...");

    CalDAV_Client_Deinit(p_Client);

    return ESP_OK;
}

esp_err_t CalDAV_List_Calendars(CalDAV_Client_t *p_Client)
{
    CalDAV_Calendar_List_t Calendars;
    ESP_LOGI(TAG, "Fetching all calendars from CalDAV server...");

    CalDAV_Error_t err = CalDAV_Calendars_List(p_Client, &Calendars);
    if (err == CALDAV_ERROR_OK) {
        ESP_LOGI(TAG, "%d calendars found:", Calendars.Length);

        for (size_t i = 0; i < Calendars.Length; i++) {
            ESP_LOGI(TAG, "\nCalendar %d:", i + 1);
            if (Calendars.Calendar[i].Name) {
                ESP_LOGI(TAG, "  Name: %s", Calendars.Calendar[i].Name);
            }

            if (Calendars.Calendar[i].DisplayName) {
                ESP_LOGI(TAG, "  Display name: %s", Calendars.Calendar[i].DisplayName);
            }

            if (Calendars.Calendar[i].Path) {
                ESP_LOGI(TAG, "  Path: %s", Calendars.Calendar[i].Path);
            }

            if (Calendars.Calendar[i].Description) {
                ESP_LOGI(TAG, "  Description: %s", Calendars.Calendar[i].Description);
            }
        }

        CalDAV_Calendars_Free(&Calendars);
    } else {
        ESP_LOGE(TAG, "Failed to retrieve calendars (Error: %d)!", err);
    }

    return ESP_OK;
}

esp_err_t CalDAV_List_Calendar_Events(CalDAV_Client_t *p_Client, const char* p_Path)
{
    CalDAV_Calendar_Event_t *events = NULL;
    size_t event_count = 0;

    ESP_LOGI(TAG, "Listing events from calendar %s", p_Path);

    /* Define time range: January 1, 2020 to December 31, 2030 */
    const char *start_time = "20200101T000000Z";
    const char *end_time = "20301231T235959Z";

    CalDAV_Error_t err = CalDAV_Calendar_Events_List(p_Client, &events, &event_count, p_Path, start_time, end_time);
    if (err == CALDAV_ERROR_OK) {
        ESP_LOGI(TAG, "✓ %d events found:", event_count);

        for (size_t i = 0; i < event_count; i++) {
            ESP_LOGI(TAG, "\nEvent %d:", i + 1);
            if (events[i].Summary) {
                ESP_LOGI(TAG, "  Title: %s", events[i].Summary);
            }

            if (events[i].Description) {
                ESP_LOGI(TAG, "  Description: %s", events[i].Description);
            }

            if (events[i].StartTime) {
                ESP_LOGI(TAG, "  Start: %s", events[i].StartTime);
            }

            if (events[i].EndTime) {
                ESP_LOGI(TAG, "  End: %s", events[i].EndTime);
            }

            if (events[i].Location) {
                ESP_LOGI(TAG, "  Location: %s", events[i].Location);
            }
        }

        /* Free events */
        CalDAV_Events_Free(events, event_count);
    } else {
        ESP_LOGE(TAG, "Failed to retrieve events (Error: %d)", err);
    }

    return ESP_OK;
}