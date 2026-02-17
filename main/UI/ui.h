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

#include "../Export/fonts/custom_fonts.h"

#define UI_MAX_EVENTS               10

lv_obj_t *UI_Dashboard_Create(uint8_t Hours_Start, uint8_t Hours_End);
void UI_Dashboard_Update(struct tm *p_TimeInfo);
void UI_Dashboard_Update_Battery(uint8_t Percentage);
void UI_Dashboard_Update_LastUpdate(struct tm *p_TimeInfo);
void UI_Dashboard_Add_Event(int DayDiff, uint8_t Hour, const char *Title,
                            const char *StartTime, const char *EndTime,
                            const char *Location, const char *Description);
void UI_Dashboard_Clear_Events(void);
void UI_Dashboard_Destroy(void);

lv_obj_t *UI_Status_Create(void);
void UI_Status_Update_Text(const char* p_Text);
void UI_Status_Update_Battery(uint8_t Percentage);
void UI_Status_Update_RSSI(int8_t RSSI);
void UI_Status_Destroy(void);

lv_obj_t *UI_DayView_Create(void);
void UI_DayView_Update_Header(struct tm *p_TimeInfo);
void UI_DayView_Update_Battery(uint8_t Percentage);
void UI_DayView_Update_RSSI(int8_t RSSI);
void UI_DayView_Add_Event(int DayDiff, uint8_t Hour, const char *Title,
                          const char *StartTime, const char *EndTime,
                          const char *Location, const char *Description);
void UI_DayView_Clear_Events(void);
void UI_DayView_Show_No_Events(void);
void UI_DayView_Destroy(void);

#endif /* UI_H_ */