#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include "esp_log.h"
#include "esp_timer.h"

#define ADC_MANAGE_PIN GPIO_NUM_5
#define ADC_READ_PIN GPIO_NUM_4
#define PWM_MANAGE_PIN GPIO_NUM_7
#define ADC_CHANNEL 3

#define LEDC_PWM_RESOLUTION LEDC_TIMER_13_BIT
#define LEDC_ADC_RESOLUTION ADC_BITWIDTH_12
#define LEDC_FREQUENCY  5000
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0

#define TIMER_PERIOD 5000000U

constexpr uint32_t maxADCResoulutionValue = (1U << LEDC_ADC_RESOLUTION) - 1; // 4095
constexpr uint32_t maxPWMResoulutionValue = (1U << LEDC_PWM_RESOLUTION) - 1; // 4095

static adc_oneshot_unit_handle_t adc_handle;

static char* LOG_TAG = "main file";

static volatile bool isTimerOn = false; 

void setup_gpio() {
    gpio_reset_pin(ADC_MANAGE_PIN);
    gpio_set_direction(ADC_MANAGE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(ADC_MANAGE_PIN, 1);
}

void setup_ledc() {
    ledc_timer_config_t timer_conf = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_PWM_RESOLUTION,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_SPEED_MODE,
        .timer_num = LEDC_TIMER
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = PWM_MANAGE_PIN,
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .hpoint = 0,
        .speed_mode = LEDC_SPEED_MODE
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void set_duty(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL));
}

void setup_adc() {
    adc_oneshot_unit_init_cfg_t adc_conf = {
        .unit_id = ADC_UNIT_1
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_conf, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = LEDC_ADC_RESOLUTION,
        .atten = ADC_ATTEN_DB_12
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));
}

uint32_t map_duty(int raw) {
    return (raw * maxPWMResoulutionValue) / maxADCResoulutionValue; 
}

void timer_callback(void* arg) {
    isTimerOn = !isTimerOn;
}

void setup_timer() {
    esp_timer_handle_t esp_timer;
    esp_timer_create_args_t args = {
        .callback = timer_callback,
        .arg = (void*) "args",
        .name = "esp_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&args, &esp_timer));

    esp_timer_start_periodic(esp_timer, TIMER_PERIOD);
}

void app_main(void)
{
    setup_gpio();
    setup_ledc();
    setup_adc();
    setup_timer();

    int adcRaw;

    while (1) {
        if (isTimerOn) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adcRaw));
            ESP_LOGI(LOG_TAG, "adc raw = %d", adcRaw);
            uint16_t duty = map_duty(adcRaw);
            ESP_LOGI(LOG_TAG, "pwm duty = %d", duty);
            set_duty(duty);
        } else {
            set_duty(0);
        }        

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
