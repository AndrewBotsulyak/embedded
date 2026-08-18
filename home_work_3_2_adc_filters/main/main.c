#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "hal/ledc_types.h"
#include "soc/clk_tree_defs.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"


#define POWER_PIN GPIO_NUM_4
#define ADC_PIN GPIO_NUM_7
#define LED_PIN GPIO_NUM_13

#define LEDC_TIMER_NUM LEDC_TIMER_0
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_FREQ 5000
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_MIN_DUTY 500

#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_RESOLUTION ADC_BITWIDTH_12
#define ADC_RAW_MAX_VALUE 3700
#define ADC_RAW_MIN_VALUE 2700
#define ADC_HYSTERESIS 100

adc_oneshot_unit_handle_t adc_unit_handle;
static char* LOG_TAG = "adc_filters";

constexpr uint32_t maxPWMResoulutionValue = (1U << LEDC_RESOLUTION) - 1; // 4095

void setup_ledc() {
    ledc_timer_config_t timer_conf = {
        .clk_cfg = LEDC_AUTO_CLK,
        .timer_num = LEDC_TIMER_NUM,
        .duty_resolution = LEDC_RESOLUTION,
        .freq_hz = LEDC_FREQ,
        .speed_mode = LEDC_MODE
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = LED_PIN,
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .hpoint = 0,
        .speed_mode = LEDC_MODE,
        .timer_sel = LEDC_TIMER_NUM
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void setup_adc() {
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_unit_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL, &config));
}

void set_duty(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// диапазод работы LEDC когда raw = 300 - 1100
// прапорционально вычисляем duty
// чем меньше raw тем больше duty (ярче LED)
// масштабирование + инверсия
uint32_t map_duty(int raw) {    
    int32_t shifted = raw - ADC_RAW_MIN_VALUE;
    uint16_t adc_range = ADC_RAW_MAX_VALUE - ADC_RAW_MIN_VALUE;
    int32_t duty = (int32_t)maxPWMResoulutionValue - (shifted * (int32_t)maxPWMResoulutionValue) / adc_range;
    if (duty > (int32_t)maxPWMResoulutionValue) 
        duty = maxPWMResoulutionValue;

    if (duty < LEDC_MIN_DUTY) {
        duty = LEDC_MIN_DUTY;
    }

    return (uint32_t)duty;
}

void check_led_conditions(int raw) {
    uint32_t duty = map_duty(raw);

    // рабочий диапазон
    if (raw >= ADC_RAW_MIN_VALUE && raw <= ADC_RAW_MAX_VALUE) {
        set_duty(duty);
    }
    // hysteresis
    else if (raw >= ADC_RAW_MIN_VALUE - ADC_HYSTERESIS && raw < ADC_RAW_MIN_VALUE) {
        set_duty(duty);
    }
    // hysteresis
    else if (raw > ADC_RAW_MAX_VALUE) {
        set_duty(LEDC_MIN_DUTY);
    }
    else if (raw < ADC_RAW_MIN_VALUE - ADC_HYSTERESIS) {
        set_duty(0);
    }

    ESP_LOGI(LOG_TAG, "adc raw = %d", raw);
    ESP_LOGI(LOG_TAG, "pwm duty = %lu", duty);
}

int sma_filter() {
    uint8_t len = 16;
    int average = 0;
    int raw;

    for(int i = 0; i < len; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_unit_handle, ADC_CHANNEL, &raw));
        average += raw;
    }

    return average / len;
}

void app_main(void)
{
    gpio_reset_pin(POWER_PIN);
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_PIN, 1);

    setup_ledc();
    setup_adc();

    int avrRaw;

    while (1) {
        
        avrRaw = sma_filter();

        check_led_conditions(avrRaw);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
