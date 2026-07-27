#pragma once

#include <Arduino.h>

class Button {
  public:
    Button(uint8_t pin);

    void init();

    bool isClicked() const;
    bool isPressed() const;
    bool inActive();

    void handleChangeState();

    void IRAM_ATTR onChange();
    static void IRAM_ATTR buttonChangeISR(void* arg);

  private:
    uint8_t _pin;
    volatile bool _isDebouncing = false;
    volatile bool _isrRequested = false;
    ulong lastTimeChecked = 0;
    ulong now = 0;
    bool _isClicked = false;
    bool _isPressed = false;
};
