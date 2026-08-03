#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "blink.h"

#define BLINK_PIN GPIO_NUM_10
#define BLINK_PERIOD_MS 200

static uint32_t gpio_state = 0;
static const char* LOG_TAG = "Blink file";
static uint32_t counter = 0;
static int64_t lastTimeChecked = 0;

void blink_setup(void) {
    gpio_reset_pin(BLINK_PIN);
    gpio_set_direction(BLINK_PIN, GPIO_MODE_OUTPUT);
}

static void blink(void) {
    gpio_set_level(BLINK_PIN, gpio_state);
    counter++;
}

void blink_reset (void) {
    gpio_state = 0;
    gpio_set_level(BLINK_PIN, gpio_state);
}

void blink_task(void)
{
    int64_t now = esp_timer_get_time() / 1000;

    if (now - lastTimeChecked >= BLINK_PERIOD_MS) {
        lastTimeChecked = now;
        blink();
        gpio_state = !gpio_state;
        ESP_LOGI(LOG_TAG, "LED state is %s, counter = %d", gpio_state == 1 ? "ON" : "OFF", counter);
        // vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}