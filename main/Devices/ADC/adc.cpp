/*
 * adc.cpp
 *
 *  Copyright (C) Daniel Kampert, 2026
 *  Website: www.kampis-elektroecke.de
 *  File info: ADC driver implementation.
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
 */

#include <stdlib.h>

#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include "adc.h"

#include <sdkconfig.h>

#define ADC_SAMPLES             16      /* Number of samples to average */
#define ADC_SAMPLE_DELAY_MS     10      /* Delay between samples in ms */
#define ADC_EMA_ALPHA           0.2f    /* Exponential moving average alpha (0.0-1.0) */
#define ADC_OUTLIER_THRESHOLD   200     /* Max deviation from median in mV */

#ifdef CONFIG_BATTERY_ADC_1
    #define ADC_UNIT    ADC_UNIT_1
#elif CONFIG_BATTERY_ADC_2
    #define ADC_UNIT    ADC_UNIT_2
#else
    #error "No ADC channel configured for battery measurement"
#endif

#ifdef CONFIG_BATTERY_ADC_ATT_0
    #define ADC_ATTEN   ADC_ATTEN_DB_0
#elif CONFIG_BATTERY_ADC_ATT_2_5
    #define ADC_ATTEN   ADC_ATTEN_DB_2_5
#elif CONFIG_BATTERY_ADC_ATT_6
    #define ADC_ATTEN   ADC_ATTEN_DB_6
#elif CONFIG_BATTERY_ADC_ATT_12
    #define ADC_ATTEN   ADC_ATTEN_DB_12
#else
    #error "No ADC attenuation configured for battery measurement"
#endif

static bool _ADC_Calib_Done = false;
static bool _ADC_Initialized = false;
static int _ADC_Last_Voltage = -1;  /* For exponential moving average */

static adc_channel_t _ADC_Channel = static_cast<adc_channel_t>(CONFIG_BATTERY_ADC_CHANNEL);
static adc_cali_handle_t _ADC_Calib_Handle;
static adc_oneshot_unit_handle_t _ADC_Handle;
static adc_oneshot_unit_init_cfg_t _ADC_Init_Config = {
    .unit_id = ADC_UNIT,
    .clk_src = ADC_RTC_CLK_SRC_RC_FAST,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

static adc_oneshot_chan_cfg_t ADC_Config = {
    .atten = static_cast<adc_atten_t>(ADC_ATTEN),
    .bitwidth = ADC_BITWIDTH_DEFAULT,
};

const char *TAG = "ADC";

esp_err_t ADC_Init(void)
{
    esp_err_t Error;

    if (_ADC_Initialized) {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&_ADC_Init_Config, &_ADC_Handle));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_ADC_Handle, _ADC_Channel, &ADC_Config));

    Error = ESP_OK;
    _ADC_Calib_Done = false;

    if (_ADC_Calib_Done == false) {
        ESP_LOGD(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t CaliConfig = {
            .unit_id = _ADC_Init_Config.unit_id,
            .chan = _ADC_Channel,
            .atten = ADC_Config.atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };

        Error = adc_cali_create_scheme_curve_fitting(&CaliConfig, &_ADC_Calib_Handle);
        if (Error == ESP_OK) {
            _ADC_Calib_Done = true;
        }
    }

    if (Error == ESP_OK) {
        ESP_LOGD(TAG, "Calibration Success");
    } else if ((Error == ESP_ERR_NOT_SUPPORTED) || (_ADC_Calib_Done == false)) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory!");
        return ESP_FAIL;
    }

    _ADC_Initialized = true;
    ESP_LOGD(TAG, "ADC Initialized");

    return ESP_OK;
}

esp_err_t ADC_Deinit(void)
{
    if (!_ADC_Initialized) {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(_ADC_Handle));

    if (_ADC_Calib_Done) {
        adc_cali_delete_scheme_curve_fitting(_ADC_Calib_Handle);
    }

    _ADC_Initialized = false;
    _ADC_Last_Voltage = -1;

    return ESP_OK;
}

/** @brief Calculate median of sorted array
 *  @param p_Array Sorted array of integers
 *  @param Size    Size of array
 *  @return        Median value
 */
static int calculate_median(int *p_Array, size_t Size)
{
    if (Size % 2 == 0) {
        return (p_Array[Size / 2 - 1] + p_Array[Size / 2]) / 2;
    } else {
        return p_Array[Size / 2];
    }
}

/** @brief Compare function for qsort
 */
static int compare_int(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

/** @brief Calculate battery percentage using improved LiPo discharge curve
 *  @param Voltage Battery voltage in mV
 *  @return        Battery percentage (0-100%)
 *  @note          Uses piecewise linear approximation of real LiPo discharge curve
 */
static uint8_t calculate_battery_percentage(int Voltage)
{
    /* LiPo discharge curve points (voltage, percentage) */
    /* These values provide better accuracy than linear interpolation */
    const struct {
        int Voltage;
        uint8_t Percentage;
    } Curve[] = {
        {3000, 0},    /* 3.0V = 0% (cutoff) */
        {3500, 5},    /* 3.5V = 5% */
        {3600, 10},   /* 3.6V = 10% */
        {3700, 25},   /* 3.7V = 25% */
        {3800, 50},   /* 3.8V = 50% */
        {3900, 70},   /* 3.9V = 70% */
        {4000, 85},   /* 4.0V = 85% */
        {4100, 95},   /* 4.1V = 95% */
        {4200, 100}   /* 4.2V = 100% (full) */
    };
    const size_t CurvePoints = sizeof(Curve) / sizeof(Curve[0]);

    /* Clamp to min/max */
    if (Voltage <= Curve[0].Voltage) {
        return 0;
    }
    if (Voltage >= Curve[CurvePoints - 1].Voltage) {
        return 100;
    }

    /* Find the two points to interpolate between */
    for (size_t i = 0; i < CurvePoints - 1; i++) {
        if (Voltage >= Curve[i].Voltage && Voltage <= Curve[i + 1].Voltage) {
            /* Linear interpolation between two points */
            int VoltageRange = Curve[i + 1].Voltage - Curve[i].Voltage;
            int PercentageRange = Curve[i + 1].Percentage - Curve[i].Percentage;
            int VoltageOffset = Voltage - Curve[i].Voltage;
            return Curve[i].Percentage + (PercentageRange * VoltageOffset / VoltageRange);
        }
    }

    return 0;
}

esp_err_t ADC_ReadBattery(int *p_Voltage, uint8_t *p_Percentage)
{
    int Raw;
    int Samples[ADC_SAMPLES];
    int Voltages[ADC_SAMPLES];
    int ValidSamples = 0;

    if ((p_Voltage == NULL) || (p_Percentage == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Initialize ADC if not already done */
    esp_err_t Error = ADC_Init();
    if (Error != ESP_OK) {
        return Error;
    }

    /* Take multiple samples for better accuracy */
    for (int i = 0; i < ADC_SAMPLES; i++) {
        Error = adc_oneshot_read(_ADC_Handle, _ADC_Channel, &Raw);
        if (Error != ESP_OK) {
            ESP_LOGW(TAG, "ADC read failed: %d", Error);
            continue;
        }

        if (_ADC_Calib_Done) {
            int Voltage;
            Error = adc_cali_raw_to_voltage(_ADC_Calib_Handle, Raw, &Voltage);
            if (Error != ESP_OK) {
                continue;
            }
            /* Apply voltage divider correction */
            Voltages[ValidSamples] = Voltage * (CONFIG_BATTERY_ADC_R1 + CONFIG_BATTERY_ADC_R2) / CONFIG_BATTERY_ADC_R2;
        } else {
            Voltages[ValidSamples] = Raw;
        }

        ValidSamples++;
        
        /* Small delay between samples */
        if (i < ADC_SAMPLES - 1) {
            vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_DELAY_MS));
        }
    }

    if (ValidSamples == 0) {
        ESP_LOGE(TAG, "No valid ADC samples obtained");
        return ESP_FAIL;
    }

    /* Sort samples for median calculation and outlier detection */
    qsort(Voltages, ValidSamples, sizeof(int), compare_int);

    /* Calculate median */
    int MedianVoltage = calculate_median(Voltages, ValidSamples);

    /* Filter outliers and calculate average of remaining samples */
    int Sum = 0;
    int Count = 0;
    for (int i = 0; i < ValidSamples; i++) {
        int Deviation = Voltages[i] - MedianVoltage;
        if (Deviation < 0) {
            Deviation = -Deviation;
        }
        
        /* Only include samples within threshold */
        if (Deviation <= ADC_OUTLIER_THRESHOLD) {
            Sum += Voltages[i];
            Count++;
        }
    }

    if (Count == 0) {
        /* If all samples were outliers, use median */
        *p_Voltage = MedianVoltage;
    } else {
        /* Use average of non-outlier samples */
        *p_Voltage = Sum / Count;
    }

    /* Apply exponential moving average for smoother readings */
    if (_ADC_Last_Voltage >= 0) {
        *p_Voltage = (int)(ADC_EMA_ALPHA * (*p_Voltage) + (1.0f - ADC_EMA_ALPHA) * _ADC_Last_Voltage);
    }
    _ADC_Last_Voltage = *p_Voltage;

    /* Calculate battery percentage using improved discharge curve */
    *p_Percentage = calculate_battery_percentage(*p_Voltage);

    ESP_LOGD(TAG, "ADC%d Channel%d: %d samples, median=%d mV, filtered=%d mV (%d%%)",
             ADC_UNIT, _ADC_Channel, ValidSamples, MedianVoltage, *p_Voltage, *p_Percentage);

    return ESP_OK;
}