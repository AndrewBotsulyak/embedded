#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"



#define RED_LIGHT_PIN GPIO_NUM_14
#define YELLOW_LIGHT_PIN GPIO_NUM_13
#define GREEN_LIGHT_PIN GPIO_NUM_12
#define PWM_PIN GPIO_NUM_4

#define LEDC_DUTY_RESOLUTION 12
#define LEDC_FREQUENCY       5000
#define LEDC_TIMER           LEDC_TIMER_0
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE

typedef enum {
    GREEN_STATE,
    GREEN_BLINK_STATE,
    YELLOW_STATE,
    RED_STATE,
    RED_YELLOW_STATE
} traffic_state;

static uint8_t state_duration[] = {
    5, // GREEN_STATE sec
    2, // GREEN_BLINK_STATE sec
    2, // YELLOW_STATE sec
    5, // RED_STATE sec
    2 // RED_YELLOW_STATE sec
};

static traffic_state current_state = GREEN_STATE;
static esp_timer_handle_t state_timer = NULL;
static esp_timer_handle_t blink_timer = NULL;
static uint8_t green_blink_state = 0;
// FUNCTIONS 

void call_next_state(void* arg);
void setup_traffic_light();
uint64_t getCurrentTimeOutUs();
void init_timers();
void call_green_blink(void* args);
void reset_lights();
void call_handle_state();
void setup_ledc();
void ledc_start_fade();
void ledc_stop_fade();

void app_main(void)
{
    setup_traffic_light();
    setup_ledc();
    init_timers();
}

void setup_ledc() {
    ledc_timer_config_t ledc_timer_conf = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_DUTY_RESOLUTION,
        .speed_mode = LEDC_SPEED_MODE,
        .freq_hz = LEDC_FREQUENCY,
        .timer_num = LEDC_TIMER
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_conf));

    ledc_channel_config_t channel_conf = {
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .hpoint = 0,
        .gpio_num = PWM_PIN,
        .speed_mode = LEDC_SPEED_MODE,
        .timer_sel = LEDC_TIMER
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    // Сервис fade ставим один раз за программу и только ПОСЛЕ channel_config:
    // install ищет уже созданный канал, чтобы получить адрес регистра прерывания
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
}

void ledc_start_fade() {
    // Плавно довести duty до 4095 за 2000 мс — железо сделает само
    ESP_ERROR_CHECK(ledc_set_fade_with_time(
    LEDC_SPEED_MODE,
    LEDC_CHANNEL,
    (1U << LEDC_DUTY_RESOLUTION) - 1,      // целевое значение duty
    2000));    // за сколько миллисекунд дойти

    // set_fade_with_time только настраивает fade, запускает его fade_start
    ESP_ERROR_CHECK(ledc_fade_start(LEDC_SPEED_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT));
}

void ledc_stop_fade() {
    // Плавно довести duty до 0 за 2000 мс — железо сделает само
    ESP_ERROR_CHECK(ledc_set_fade_with_time(
    LEDC_SPEED_MODE,
    LEDC_CHANNEL,
    0,      // целевое значение duty
    2000));    // за сколько миллисекунд дойти

    ESP_ERROR_CHECK(ledc_fade_start(LEDC_SPEED_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT));
}

void setup_traffic_light() {
    gpio_reset_pin(RED_LIGHT_PIN);
    gpio_reset_pin(YELLOW_LIGHT_PIN);
    gpio_reset_pin(GREEN_LIGHT_PIN);

    gpio_set_direction(RED_LIGHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(YELLOW_LIGHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LIGHT_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(RED_LIGHT_PIN, 0);
    gpio_set_level(YELLOW_LIGHT_PIN, 0);
    gpio_set_level(GREEN_LIGHT_PIN, 0);
}

void call_next_state(void* arg) {
    reset_lights();

    // ESP_LOGI()

    switch (current_state) {
        case GREEN_STATE:
            current_state = GREEN_BLINK_STATE;
        break;
        case GREEN_BLINK_STATE:
            current_state = YELLOW_STATE;
        break;
        case YELLOW_STATE:
            current_state = RED_STATE;
        break;
        case RED_STATE:
            current_state = RED_YELLOW_STATE;
        break;
        case RED_YELLOW_STATE:
            current_state = GREEN_STATE;
        break;
    }

    call_handle_state();

    ESP_ERROR_CHECK(esp_timer_start_once(state_timer, getCurrentTimeOutUs()));

    esp_timer_stop(blink_timer);
    if (current_state == GREEN_BLINK_STATE) {
        ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, 500000));
    }
}

uint64_t getCurrentTimeOutUs() {
    return (uint64_t)state_duration[current_state] * 1000000U;
}

void call_green_blink(void* args) {
    green_blink_state = !green_blink_state;
    gpio_set_level(GREEN_LIGHT_PIN, green_blink_state);
}

void call_handle_state() {
    switch (current_state) {
        case GREEN_STATE:
            gpio_set_level(GREEN_LIGHT_PIN, 1);
            ledc_start_fade();
        break;
        case YELLOW_STATE:
            gpio_set_level(YELLOW_LIGHT_PIN, 1);
            ledc_stop_fade();
        break;
        case RED_STATE:
            gpio_set_level(RED_LIGHT_PIN, 1);
        break;
        case RED_YELLOW_STATE:
            gpio_set_level(YELLOW_LIGHT_PIN, 1);
            gpio_set_level(RED_LIGHT_PIN, 1);
        break;
        default:
         return;
    }
}

void reset_lights() {
    gpio_set_level(GREEN_LIGHT_PIN, 0);
    green_blink_state = 0;
    gpio_set_level(YELLOW_LIGHT_PIN, 0);
    gpio_set_level(RED_LIGHT_PIN, 0);
}

void init_timers() {
    esp_timer_create_args_t create_args = {
        .callback = call_next_state,
        .name = "esp_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&create_args, &state_timer));

    ESP_ERROR_CHECK(esp_timer_start_once(state_timer, getCurrentTimeOutUs()));

    gpio_set_level(GREEN_LIGHT_PIN, 1);

    // GREEN_BLINK TIMER
    esp_timer_create_args_t create_args_blink = {
        .callback = call_green_blink,
        .name = "esp_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&create_args_blink, &blink_timer));
}
