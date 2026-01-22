/*
 * bm8563.h
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: BM8563 RTC driver.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Errors and commissions should be reported to DanielKampert@kampis-elektroecke.de
 */

#ifndef BM8563_H_
#define BM8563_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>

/** @brief              Initialize BM8563 RTC.
 *  @param BusHandle    I2C master bus handle
 *  @param p_Handle     Pointer to I2C device handle
 *  @return             ESP_OK on success
 */
esp_err_t BM8563_Init(i2c_master_bus_handle_t BusHandle, i2c_master_dev_handle_t *p_Handle);

/** @brief          Deinitialize BM8563 RTC.
 *  @param p_Handle Pointer to I2C device handle
 *  @return         ESP_OK on success
 */
esp_err_t BM8563_Deinit(i2c_master_dev_handle_t *p_Handle);

/** @brief              Read time from BM8563.
 *  @param Handle       I2C device handle
 *  @param p_TimeInfo   Pointer to tm structure to store time
 *  @return             ESP_OK on success
 */
esp_err_t BM8563_GetTime(i2c_master_dev_handle_t Handle, struct tm *p_TimeInfo);

/** @brief              Write time to BM8563.
 *  @param Handle       I2C device handle
 *  @param p_TimeInfo   Pointer to tm structure with time to set
 *  @return             ESP_OK on success
 */
esp_err_t BM8563_SetTime(i2c_master_dev_handle_t Handle, const struct tm *p_TimeInfo);

/** @brief          Check if RTC has valid time.
 *  @param Handle   I2C device handle
 *  @return         true if RTC has valid time
 */
bool BM8563_IsValid(i2c_master_dev_handle_t Handle);

#endif /* BM8563_H_ */
