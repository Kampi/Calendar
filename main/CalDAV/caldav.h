/*
 * caldav.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: CalDAV client handling.
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

#ifndef CALDAV_H_
#define CALDAV_H_

#include <esp_err.h>

#include "caldav_client.h"

/** @brief  Initialize CalDAV client
 *  @return CalDAV client handle or NULL on failure
 */
CalDAV_Client_t *CalDAV_Init(void);

/** @brief
 *  @param p_Client
 *  @return
 */
esp_err_t CalDAV_Deinit(CalDAV_Client_t *p_Client);

/** @brief
 *  @param p_Client
 *  @return
 */
esp_err_t CalDAV_List_Calendars(CalDAV_Client_t *p_Client);

/** @brief
 *  @param p_Client
 *  @param p_Path
 *  @return
 */
esp_err_t CalDAV_List_Calendar_Events(CalDAV_Client_t *p_Client, const char* p_Path);

#endif /* CALDAV_H_ */