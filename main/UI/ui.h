/*
 * ui.h
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

#ifndef UI_H_
#define UI_H_

#include <lvgl.h>
#include <time.h>

void UI_Dashboard_Create(uint8_t Hours_Start, uint8_t Hours_End);
void UI_Dashboard_Update_Time(struct tm *timeinfo);
void UI_Dashboard_Update_Battery(uint8_t Percentage);
void UI_Dashboard_Add_Event(const char *day, uint8_t hour, const char *title, 
                           const char *start_time, const char *end_time,
                           const char *location, const char *description);
void UI_Dashboard_Clear_Events(void);

#endif /* UI_H_ */