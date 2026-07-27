#pragma once

#include <Arduino.h>

#include <led/led.h>

class Config {
  public:
    static constexpr uint8_t LED_PIN = 11;
    static constexpr uint8_t LDR_PIN = 7;
    static constexpr uint8_t ADC_PIN = 5;
    static constexpr uint8_t LEFT_BTN = 8;
    static constexpr uint8_t RIGHT_BTN = 18;
    static constexpr uint8_t DEBOUNCE_DELAY = 20;
    static constexpr uint16_t BLINK_DELAY = 500;

    static constexpr LedADCOptions ledADCOptions = ::ledADCOptions;

    static constexpr uint16_t minLedBrightness = 1;
    static constexpr uint16_t maxLedBrightness = (1 << ledADCOptions.resolution) - 1; // 4095
    static constexpr uint16_t ledBrightnessPercentValue = (maxLedBrightness - minLedBrightness) / 100;
    static constexpr uint16_t ledLightStep = ledBrightnessPercentValue * 10;

    static constexpr uint16_t maxAdcDuty = (1 << ledADCOptions.resolution) - 1; // 4095
    static constexpr uint16_t adcDutyTreshold = 2700;
    static constexpr uint16_t minDutyTreshold = 2700 - 200;
    static constexpr uint16_t dutyOneStep = (maxAdcDuty - adcDutyTreshold) / 100; // one percent
};
