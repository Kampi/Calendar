/*
 * custom_fonts.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: Extended Montserrat fonts with German umlaut support (U+00C0-U+00FF).
 *             Each font declares lv_font_montserrat_XX as fallback, so all ASCII
 *             characters and FontAwesome symbols are resolved via the fallback chain.
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

#ifndef CUSTOM_FONTS_H_
#define CUSTOM_FONTS_H_

#include <lvgl.h>

/* Extended Montserrat fonts: contain U+00C0-U+00FF (umlauts etc.) generated from
 * Montserrat-Medium.ttf to match the weight of the built-in lv_font_montserrat_XX fonts.
 * Fall back to lv_font_montserrat_XX for ASCII and FontAwesome symbols. */
LV_FONT_DECLARE(lv_font_montserrat_ext_12)
LV_FONT_DECLARE(lv_font_montserrat_ext_14)
LV_FONT_DECLARE(lv_font_montserrat_ext_18)
LV_FONT_DECLARE(lv_font_montserrat_ext_20)
LV_FONT_DECLARE(lv_font_montserrat_ext_22)
LV_FONT_DECLARE(lv_font_montserrat_ext_24)
LV_FONT_DECLARE(lv_font_montserrat_ext_28)
LV_FONT_DECLARE(lv_font_montserrat_ext_40)

#endif /* CUSTOM_FONTS_H_ */
