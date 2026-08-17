#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "hal/adc_types.h"

#define POWER_PIN GPIO_NUM_4
#define ADC_PIN GPIO_NUM_7
#define ADC_RESOLUTION ADC_BITWIDTH_12

#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_READ_DELAY 1000
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_UNIT ADC_UNIT_1
#define ADC_VREF_MV 3300.0f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;

static char* TAG_LOG = "adc-cali";
constexpr uint16_t ADC_RAW_MAX = (1U << ADC_RESOLUTION) - 1;

void setup_adc() {
    adc_oneshot_unit_init_cfg_t adc_conf = {
        .unit_id = ADC_UNIT
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_conf, &adc_handle));

    adc_oneshot_chan_cfg_t channel_conf = {
        .bitwidth = ADC_RESOLUTION,
        .atten = ADC_ATTEN
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &channel_conf));

    
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_RESOLUTION,
        .chan = ADC_CHANNEL
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));

}

// напряжение (мВ), рассчитанное вручную из предположения линейной шкалы 0..VREF
float_t get_adc_voltage_mv(int raw) {
    return ((float_t)raw / ADC_RAW_MAX) * ADC_VREF_MV;
}

// погрешность ручного расчёта относительно калиброванного значения, %
float_t get_error_percent(float_t manual_mv, int cali_mv) {
    if (cali_mv == 0) {
        return 0.0f;
    }
    return fabsf(manual_mv - cali_mv) / cali_mv * 100.0f;
}

void app_main(void)
{
    // init pins
    ESP_ERROR_CHECK(gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(POWER_PIN, 1));

    setup_adc();

    int rawData;
    int cali_mv;

    ESP_LOGI(TAG_LOG, "start measuring, VREF = %.0f mV, raw max = %u", ADC_VREF_MV, ADC_RAW_MAX);

    printf("\n%-6s %-14s %-12s %-9s\n", "RAW", "U_manual(mV)", "U_cali(mV)", "Error(%)");

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &rawData));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, rawData, &cali_mv));

        float_t manual_mv = get_adc_voltage_mv(rawData);

        printf("%-6d %-14.1f %-12d %-9.2f\n",
               rawData, manual_mv, cali_mv, get_error_percent(manual_mv, cali_mv));

        vTaskDelay(pdMS_TO_TICKS(ADC_READ_DELAY));
    }

}
