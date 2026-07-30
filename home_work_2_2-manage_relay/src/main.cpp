#include <Arduino.h>

#include "task1_relay.h"
#include "task2_motor.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // setupTask1();
  setupTask2();
}

void loop() {
  // runTask1();
  runTask2();
}
