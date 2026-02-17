/*
 * bm8563.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: BM8563 RTC driver implementation.
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

#include <esp_log.h>
#include <string.h>

#include "bm8563.h"
#include "../I2C/i2c.h"

/** BM8563 I2C address */
#define BM8563_I2C_ADDRESS          0x51

/** BM8563 register addresses */
#define BM8563_REG_CONTROL_STATUS1  0x00
#define BM8563_REG_CONTROL_STATUS2  0x01
#define BM8563_REG_SECONDS          0x02
#define BM8563_REG_MINUTES          0x03
#define BM8563_REG_HOURS            0x04
#define BM8563_REG_DAYS             0x05
#define BM8563_REG_WEEKDAYS         0x06
#define BM8563_REG_MONTHS           0x07
#define BM8563_REG_YEARS            0x08
#define BM8563_REG_ALARM_MINUTES    0x09
#define BM8563_REG_ALARM_HOURS      0x0A
#define BM8563_REG_ALARM_DAYS       0x0B
#define BM8563_REG_ALARM_WEEKDAYS   0x0C

/** Control Status 2 bits */
#define BM8563_CTL2_AIE             (1 << 1)  /**< Alarm Interrupt Enable */
#define BM8563_CTL2_AF              (1 << 3)  /**< Alarm Flag */

/** Alarm register bit 7 - when set, alarm for this field is disabled */
#define BM8563_ALARM_DISABLE        (1 << 7)

static const char *TAG = "BM8563";

/** @brief          Convert BCD to binary
 *  @param Value    BCD value
 *  @return         Binary value
 */
static uint8_t bcd_to_bin(uint8_t Value)
{
    return ((Value >> 4) * 10) + (Value & 0x0F);
}

/** @brief          Convert binary to BCD
 *  @param Value    Binary value
 *  @return         BCD value
 */
static uint8_t bin_to_bcd(uint8_t Value)
{
    return ((Value / 10) << 4) | (Value % 10);
}

/** @brief          Write to BM8563 register
 *  @param p_Handle Pointer to I2C device handle
 *  @param Reg      Register address
 *  @param Data     Data to write
 *  @return         ESP_OK on success
 */
static esp_err_t bm8563_write_reg(i2c_master_dev_handle_t *p_Handle, uint8_t Reg, uint8_t Data)
{
    uint8_t WriteBuf[2] = {Reg, Data};

    return I2CM_Write(p_Handle, WriteBuf, 2);
}

/** @brief          Read from BM8563 register
 *  @param p_Handle Pointer to I2C device handle
 *  @param Reg      Register address
 *  @param p_Data   Pointer to store read data
 *  @return         ESP_OK on success
 */
static esp_err_t bm8563_read_reg(i2c_master_dev_handle_t *p_Handle, uint8_t Reg, uint8_t *p_Data)
{
    return I2CM_WriteRead(p_Handle, &Reg, 1, p_Data, 1);
}

/** @brief          Read multiple bytes from BM8563
 *  @param p_Handle Pointer to I2C device handle
 *  @param Reg      Starting register address
 *  @param p_Data   Pointer to store read data
 *  @param Len      Number of bytes to read
 *  @return         ESP_OK on success
 */
static esp_err_t bm8563_read_bytes(i2c_master_dev_handle_t *p_Handle, uint8_t Reg, uint8_t *p_Data, size_t Len)
{
    return I2CM_WriteRead(p_Handle, &Reg, 1, p_Data, Len);
}

esp_err_t BM8563_Init(i2c_master_bus_handle_t BusHandle, i2c_master_dev_handle_t *p_Handle)
{
    esp_err_t Error;
    i2c_device_config_t DevConfig;

    memset(&DevConfig, 0, sizeof(DevConfig));

    /* Add BM8563 device to bus */
    DevConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    DevConfig.device_address = BM8563_I2C_ADDRESS;
    DevConfig.scl_speed_hz = 400000;

    Error = i2c_master_bus_add_device(BusHandle, &DevConfig, p_Handle);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %d!", Error);

        return Error;
    }

    /* Clear control status registers */
    Error = bm8563_write_reg(p_Handle, BM8563_REG_CONTROL_STATUS1, 0x00);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BM8563!");
        return Error;
    }

    Error = bm8563_write_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, 0x00);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BM8563!");
        return Error;
    }

    ESP_LOGD(TAG, "BM8563 RTC initialized successfully");

    return ESP_OK;
}

esp_err_t BM8563_Deinit(i2c_master_dev_handle_t *p_Handle)
{
    if (p_Handle) {
        i2c_master_bus_rm_device(*p_Handle);
        *p_Handle = NULL;
    }

    return ESP_OK;
}

esp_err_t BM8563_GetTime(i2c_master_dev_handle_t *p_Handle, struct tm *p_TimeInfo)
{
    if (p_TimeInfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t Data[7];
    esp_err_t Error = bm8563_read_bytes(p_Handle, BM8563_REG_SECONDS, Data, 7);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time from BM8563!");
        return Error;
    }

    /* Check if clock integrity is guaranteed (VL bit) */
    if (Data[0] & 0x80) {
        ESP_LOGW(TAG, "RTC clock integrity not guaranteed (VL bit set)");
    }

    memset(p_TimeInfo, 0, sizeof(struct tm));

    p_TimeInfo->tm_sec = bcd_to_bin(Data[0] & 0x7F);
    p_TimeInfo->tm_min = bcd_to_bin(Data[1] & 0x7F);
    p_TimeInfo->tm_hour = bcd_to_bin(Data[2] & 0x3F);
    p_TimeInfo->tm_mday = bcd_to_bin(Data[3] & 0x3F);
    p_TimeInfo->tm_wday = bcd_to_bin(Data[4] & 0x07);
    p_TimeInfo->tm_mon = bcd_to_bin(Data[5] & 0x1F) - 1;    /* tm_mon is 0-11 */
    p_TimeInfo->tm_year = bcd_to_bin(Data[6]) + 100;        /* tm_year is years since 1900 */

    /* Check if century bit is set */
    if (Data[5] & 0x80) {
        p_TimeInfo->tm_year += 100;  /* 21st century */
    }

    return ESP_OK;
}

esp_err_t BM8563_SetTime(i2c_master_dev_handle_t *p_Handle, const struct tm *p_TimeInfo)
{
    if (p_TimeInfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t Data[8];
    Data[0] = BM8563_REG_SECONDS;
    Data[1] = bin_to_bcd(p_TimeInfo->tm_sec);
    Data[2] = bin_to_bcd(p_TimeInfo->tm_min);
    Data[3] = bin_to_bcd(p_TimeInfo->tm_hour);
    Data[4] = bin_to_bcd(p_TimeInfo->tm_mday);
    Data[5] = bin_to_bcd(p_TimeInfo->tm_wday);

    /* Handle century bit */
    int Year = p_TimeInfo->tm_year - 100;  /* Convert from years since 1900 */
    uint8_t Century = 0;
    if (Year >= 100) {
        Century = 0x80;
        Year -= 100;
    }

    Data[6] = bin_to_bcd(p_TimeInfo->tm_mon + 1) | Century;  /* tm_mon is 0-11 */
    Data[7] = bin_to_bcd(Year);

    esp_err_t Error = I2CM_Write(p_Handle, Data, 8);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set time on BM8563!");
        return Error;
    }

    ESP_LOGD(TAG, "RTC time set: %04d-%02d-%02d %02d:%02d:%02d",
             p_TimeInfo->tm_year + 1900, p_TimeInfo->tm_mon + 1, p_TimeInfo->tm_mday,
             p_TimeInfo->tm_hour, p_TimeInfo->tm_min, p_TimeInfo->tm_sec);

    return ESP_OK;
}

bool BM8563_IsValid(i2c_master_dev_handle_t *p_Handle)
{
    uint8_t Data;
    esp_err_t Error = bm8563_read_reg(p_Handle, BM8563_REG_SECONDS, &Data);

    if (Error != ESP_OK) {
        return false;
    }

    /* Check VL bit (bit 7) - if set, clock integrity is not guaranteed */
    return (Data & 0x80) == 0;
}

esp_err_t BM8563_SetAlarm(i2c_master_dev_handle_t *p_Handle, const struct tm *p_TimeInfo)
{
    esp_err_t Error;
    uint8_t Data[5];
    uint8_t Ctl2;

    if (p_TimeInfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Prepare alarm registers */
    Data[0] = BM8563_REG_ALARM_MINUTES;

    /* Minute alarm - always enabled for time-based alarm */
    if ((p_TimeInfo->tm_min < 0) || (p_TimeInfo->tm_min > 59)) {
        Data[1] = BM8563_ALARM_DISABLE;
    } else {
        Data[1] = bin_to_bcd(p_TimeInfo->tm_min) & 0x7F;
    }

    /* Hour alarm - always enabled for time-based alarm */
    if ((p_TimeInfo->tm_hour < 0) || (p_TimeInfo->tm_hour > 23)) {
        Data[2] = BM8563_ALARM_DISABLE;
    } else {
        Data[2] = bin_to_bcd(p_TimeInfo->tm_hour) & 0x3F;
    }

    /* Day alarm - optional (disable if < 1) */
    if ((p_TimeInfo->tm_mday < 1) || (p_TimeInfo->tm_mday > 31)) {
        Data[3] = BM8563_ALARM_DISABLE;
    } else {
        Data[3] = bin_to_bcd(p_TimeInfo->tm_mday) & 0x3F;
    }

    /* Weekday alarm - optional (disable if < 0) */
    if ((p_TimeInfo->tm_wday < 0) || (p_TimeInfo->tm_wday > 6)) {
        Data[4] = BM8563_ALARM_DISABLE;
    } else {
        Data[4] = bin_to_bcd(p_TimeInfo->tm_wday) & 0x07;
    }

    /* Write alarm registers */
    Error = I2CM_Write(p_Handle, Data, 5);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write alarm registers!");

        return Error;
    }

    /* Enable alarm interrupt in Control Status 2 */
    Error = bm8563_read_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, &Ctl2);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read control status 2!");

        return Error;
    }

    /* Set AIE bit to enable alarm interrupt */
    Ctl2 |= BM8563_CTL2_AIE;

    /* Clear AF bit to clear any pending alarm */
    Ctl2 &= ~BM8563_CTL2_AF;

    Error = bm8563_write_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, Ctl2);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable alarm interrupt!");

        return Error;
    }

    ESP_LOGI(TAG, "Alarm set: %02d:%02d on day %d (weekday %d)",
             p_TimeInfo->tm_hour >= 0 ? p_TimeInfo->tm_hour : -1,
             p_TimeInfo->tm_min >= 0 ? p_TimeInfo->tm_min : -1,
             p_TimeInfo->tm_mday >= 1 ? p_TimeInfo->tm_mday : -1,
             p_TimeInfo->tm_wday >= 0 ? p_TimeInfo->tm_wday : -1);

    return ESP_OK;
}

esp_err_t BM8563_ClearAlarm(i2c_master_dev_handle_t *p_Handle)
{
    esp_err_t Error;
    uint8_t Data[5];
    uint8_t Ctl2;

    /* Disable all alarm fields by setting AE bit */
    Data[0] = BM8563_REG_ALARM_MINUTES;
    Data[1] = BM8563_ALARM_DISABLE;
    Data[2] = BM8563_ALARM_DISABLE;
    Data[3] = BM8563_ALARM_DISABLE;
    Data[4] = BM8563_ALARM_DISABLE;

    Error = I2CM_Write(p_Handle, Data, 5);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable alarm!");
        return Error;
    }

    /* Disable alarm interrupt in Control Status 2 */
    Error = bm8563_read_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, &Ctl2);
    if (Error != ESP_OK) {
        return Error;
    }

    /* Clear AIE and AF bits */
    Ctl2 &= ~(BM8563_CTL2_AIE | BM8563_CTL2_AF);

    Error = bm8563_write_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, Ctl2);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable alarm interrupt!");
        return Error;
    }

    ESP_LOGD(TAG, "Alarm disabled");

    return ESP_OK;
}

esp_err_t BM8563_ClearAlarmFlag(i2c_master_dev_handle_t *p_Handle)
{
    esp_err_t Error;
    uint8_t Ctl2;

    /* Read Control Status 2 */
    Error = bm8563_read_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, &Ctl2);
    if (Error != ESP_OK) {
        return Error;
    }

    /* Clear AF bit (alarm flag) */
    Ctl2 &= ~BM8563_CTL2_AF;

    Error = bm8563_write_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, Ctl2);
    if (Error != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear alarm flag!");
        return Error;
    }

    ESP_LOGD(TAG, "Alarm flag cleared");

    return ESP_OK;
}

bool BM8563_IsAlarmActive(i2c_master_dev_handle_t *p_Handle)
{
    uint8_t Ctl2;
    esp_err_t Error = bm8563_read_reg(p_Handle, BM8563_REG_CONTROL_STATUS2, &Ctl2);

    if (Error != ESP_OK) {
        return false;
    }

    /* Check if AF bit (alarm flag) is set */
    return (Ctl2 & BM8563_CTL2_AF) != 0;
}
