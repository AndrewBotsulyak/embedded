#pragma once

#include <led/led.h>
#include <button/button.h>

class AppControl {
  public:
    AppControl(LedADC& led, Button& leftButton, Button& rightButton);

    void setModeEditing(bool value);
    bool isModeEditing();
    
    void handleButtonsLogic();

  private:
    LedADC& _led;
    Button& _leftButton;
    Button& _rightButton;
    bool _isModeEditing = false;
};
