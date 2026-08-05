#include <stdio.h>
#include <stdbool.h>

#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_10
#define RESOULUTION_HZ 1000000
#define ALARM_TRESHOLD 500000

static gptimer_handle_t gpTimerInstance;
static char* LOG_TAG = "timer";

volatile uint32_t counter = 0;
volatile int led_state = 0;

void setup_led() {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, led_state);
}

static bool IRAM_ATTR on_timer_alarm(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx
) {
    led_state = !led_state;

    gpio_set_level(LED_PIN, led_state);

    counter++;

    return false;
}

void setup_timer() {
    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = RESOULUTION_HZ
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &gpTimerInstance));
    

    // ADD ISR
    gptimer_event_callbacks_t cbs = {
        .on_alarm = on_timer_alarm
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gpTimerInstance, &cbs, NULL));

    gptimer_alarm_config_t alrm_config = {
        .alarm_count = ALARM_TRESHOLD, // 500 000 мкс = 500 мс
        .reload_count = 0,              // при срабатывании сбросить счётчик в 0
        .flags.auto_reload_on_alarm = true // авто-перезапуск
    };
    
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gpTimerInstance, &alrm_config));

    ESP_ERROR_CHECK(gptimer_enable(gpTimerInstance));
    ESP_ERROR_CHECK(gptimer_start(gpTimerInstance));
}


void app_main(void)
{
    setup_led();
    setup_timer();


    while(1) {
        ESP_LOGI(LOG_TAG, "counter = %d, led_state = %d", counter, led_state);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
