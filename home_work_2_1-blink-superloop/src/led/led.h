#pragma once

#include <Arduino.h>

enum class LedMode : uint8_t {
  LDR,
  CONSTANT_LIGHT,
  BLINK
};

typedef struct {
  uint8_t channel;
  uint32_t frequency;
  uint8_t resolution;
} LedADCOptions;

constexpr LedADCOptions ledADCOptions {
  .channel    = 0,
  .frequency  = 5000,
  .resolution = 12,
};

class LedADC {
  public:
    LedADC(uint8_t pin, LedADCOptions options);

    void init();

    void setMode(LedMode mode);
    LedMode getMode() const;

    void handleLDRDuty(uint16_t rawDuty);
    void handleBlink();
    void nextMode();
    void handleBehavior(bool isIncrease = false);

  private:
    uint8_t _pin;
    LedMode _mode;
    LedADCOptions _options;
    uint16_t configuredDutyLight = 0;

    void dutyWrite(uint16_t duty);
    uint32_t getCurrentDuty();
};
