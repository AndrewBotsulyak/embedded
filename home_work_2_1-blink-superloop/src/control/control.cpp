#include <control/control.h>

AppControl::AppControl(LedADC& led, Button& leftBtn, Button& rightBtn)
  : _led(led),
    _leftButton(leftBtn),
    _rightButton(rightBtn) {}

bool AppControl::isModeEditing() {
    return _isModeEditing;
}

void AppControl::setModeEditing(bool value) {
    _isModeEditing = value;
}

void AppControl::handleButtonsLogic() {
  static bool isChangeModeHandled = false;

  // handle button clicks
  if (isModeEditing() == false) {
    if (_leftButton.isClicked()) {
        _led.handleBehavior(true);
    }
    else if (_rightButton.isClicked()) {
        _led.handleBehavior();
    }
  }
  // handle when mode is changing
  if (_leftButton.inActive() && _rightButton.inActive()) {
        setModeEditing(false);
    }
    else if ((_leftButton.isPressed() && _rightButton.isClicked()) 
    || (_leftButton.isClicked() && _rightButton.isPressed())) {
        _led.nextMode();
        setModeEditing(true);
    }
}
