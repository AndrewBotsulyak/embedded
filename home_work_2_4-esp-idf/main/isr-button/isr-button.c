#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "isr-button.h"
#include "../blink/blink.h"

// #define BUTTON_PIN 17
#define DEBOUNCE_DELAY 20

static const char* LOG_TAG = "isr-button file";

static void IRAM_ATTR isrButtonClick(void* btn) {
    DebouncedBtn* button = (DebouncedBtn*) btn;
    button->isDebouncing = true;
    button->isrRequested = true;
}

void isrButtonInit(DebouncedBtn* btn) {
    // 1. Настроить ножку как вход с прерыванием (по спадающему фронту)
    gpio_config_t buttonConf = {
        .pin_bit_mask = (1ULL << btn->pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    gpio_config(&buttonConf);

    esp_err_t err = gpio_isr_handler_add(btn->pin, isrButtonClick, (void*) btn);

    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "gpio_isr_handler_add returned error");
    }
}

static int64_t getTime() {
    return esp_timer_get_time() / 1000;
}

void handleDebounce(DebouncedBtn* btn) {
    if (btn->isDebouncing == false) {
        return;
    }

  // если все еще в процесе устранения дребезга
  if (btn->isrRequested == true) {
    btn->lastTimeChecked = getTime();
    btn->isrRequested = false;
    ESP_LOGI(LOG_TAG, "btn->isrRequested == true");
  }
  // если начали отсчет и кнопка нажата
  else if(btn->isrRequested == false && gpio_get_level(btn->pin) == 0) { // if LOW
    int64_t now = esp_timer_get_time() / 1000;

    if (now - btn->lastTimeChecked > DEBOUNCE_DELAY) {

        // Handle Click
        // Main Logic
        btn->isPressed = true;
      
      ESP_LOGI(LOG_TAG, "Button Clicked pin: %d", btn->pin);
      btn->isDebouncing = false; // finish handling debouncing
    }
  }
  else if (btn->isrRequested == false && gpio_get_level(btn->pin) == 1) { // if HIGH
    int64_t now = getTime();
    if (now - btn->lastTimeChecked > DEBOUNCE_DELAY) {
      
      ESP_LOGI(LOG_TAG, "Button is unPressed");
      btn->isDebouncing = false;
      btn->isPressed = false;
    }
  }
}

DebouncedBtn getDefaultBtn(gpio_num_t pinNumber) {
    return (DebouncedBtn) {
        .pin = pinNumber,
        .isDebouncing = false,
        .isPressed = false,
        .isrRequested = false,
        .lastTimeChecked = 0
    };
}


