#include <Arduino.h>

#include <config/config.h>
#include <led/led.h>
#include <button/button.h>
#include <control/control.h>

// VARIABLES
LedADC LED(Config::LED_PIN, Config::ledADCOptions);
Button leftButton(Config::LEFT_BTN);
Button rightButton(Config::RIGHT_BTN);
AppControl appControl(LED, leftButton, rightButton);

void setup() {
  Serial.begin(115200);

  // initialize all logic for LED
  LED.init();

  // initialize all logic for buttons
  leftButton.init();
  rightButton.init();
  pinMode(Config::LDR_PIN, OUTPUT);
  digitalWrite(Config::LDR_PIN, HIGH);
}

void loop() {
  leftButton.handleChangeState();
  rightButton.handleChangeState();

  if (LED.getMode() == LedMode::LDR) {
    LED.handleLDRDuty(analogRead(Config::ADC_PIN));
  }
  else if (LED.getMode() == LedMode::BLINK) {
    LED.handleBlink();
  }

  appControl.handleButtonsLogic();
}
