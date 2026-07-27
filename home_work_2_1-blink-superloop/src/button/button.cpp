#include <button/button.h>

#include <config/config.h>

Button::Button(uint8_t pin) : _pin(pin) {}

void Button::init() {
  pinMode(_pin, INPUT_PULLUP);
  attachInterruptArg(digitalPinToInterrupt(_pin), Button::buttonChangeISR, this, CHANGE);
}

bool Button::isClicked() const {
  return _isClicked;
}

bool Button::isPressed() const {
  return _isPressed;
}

bool Button::inActive() {
  return isClicked() == false && isPressed() == false;
}

void Button::handleChangeState() {
  if (_isDebouncing == true) {
    if (_isrRequested == true) {
      lastTimeChecked = millis();
      _isrRequested = false;
    }
    else if (_isrRequested == false && digitalRead(_pin) == LOW && _isPressed == false) {
      now = millis();

      // rising edge logic
      // main flag - button has stable state
      if (now - lastTimeChecked > Config::DEBOUNCE_DELAY) {
        _isPressed = true;
      }
    }
    else if (_isrRequested == false && digitalRead(_pin) == HIGH) {
      now = millis();

      // logic on falling edge
      if (now - lastTimeChecked > Config::DEBOUNCE_DELAY) {
        _isDebouncing = false; // finish debouncing logic
        _isClicked = true;     // mark button as clicked
        _isPressed = false;
      }
    }
  } else {
    _isClicked = false;
  }
}

void IRAM_ATTR Button::onChange() {
  _isrRequested = true;
  _isDebouncing = true;
}

void IRAM_ATTR Button::buttonChangeISR(void* arg) {
  static_cast<Button*>(arg)->onChange();
}
