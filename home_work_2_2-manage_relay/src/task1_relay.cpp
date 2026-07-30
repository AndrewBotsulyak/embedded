#include <Arduino.h>
#include "task1_relay.h"

#define RELAY_MANAGE_PIN 7
#define RELAY_STATE_PIN 4
#define POWER_PIN 10

#define COUNT_RESULTS 10

// VARIABLES
static volatile bool isRelayOn = false;
static volatile bool isFirstFalling = true;
static volatile ulong isrTime = 0;
static volatile ulong isrTimeMks = 0;
static ulong actionTimeMks = 0;
static ulong actionTime = 0;
static bool isRelaySetted = false;
static uint32_t count = 0;
static ulong arrResult[COUNT_RESULTS];
static ulong arrResultMks[COUNT_RESULTS];

// ISRs
static void IRAM_ATTR handleRelayOn();

void setupTask1() {
  pinMode(RELAY_MANAGE_PIN, OUTPUT);
  pinMode(RELAY_STATE_PIN, INPUT_PULLDOWN);
  pinMode(POWER_PIN, OUTPUT);

  digitalWrite(RELAY_MANAGE_PIN, HIGH); // OFF
  digitalWrite(POWER_PIN, HIGH);

  Serial.printf("digitalWrite(RELAY_MANAGE_PIN, HIGH); \n");

  delay(1000);

  attachInterrupt(digitalPinToInterrupt(RELAY_STATE_PIN), handleRelayOn, FALLING);
}

void runTask1() {

  if (count < COUNT_RESULTS) {

    // if we have already set pin to LOW
    if (isRelaySetted == false) {
      isRelaySetted = true;

      Serial.printf("digitalWrite(RELAY_MANAGE_PIN, LOW); \n");

      digitalWrite(RELAY_MANAGE_PIN, LOW); // ON
      actionTimeMks = micros();
      actionTime = millis();
    }

    if (isRelayOn == true) {

        arrResultMks[count] = isrTimeMks - actionTimeMks;
        arrResult[count] = isrTime - actionTime;

        Serial.printf("%u check = %lu \n", count, arrResult[count]);

        count++;
        isRelaySetted = false;
        isRelayOn = false;
        delay(1000);

        digitalWrite(RELAY_MANAGE_PIN, HIGH); // OFF

        Serial.printf("digitalWrite(RELAY_MANAGE_PIN, HIGH); \n");
        delay(1000);
        isFirstFalling = true;
      }
  }
  else if (count == COUNT_RESULTS) {
    Serial.printf("millis array ... \n");
    for (size_t i = 0; i < COUNT_RESULTS; i++) {
      Serial.printf("%lu - ", arrResult[i]);
    }
    Serial.printf("\n micros array ... \n");
    for (size_t i = 0; i < COUNT_RESULTS; i++) {
      Serial.printf("%lu - ", arrResultMks[i]);
    }
    Serial.printf("\n");
    count++;
  }
}


static void IRAM_ATTR handleRelayOn() {
  if (isFirstFalling == true) {
    isrTimeMks = micros();
    isrTime = millis();
    isRelayOn = true;
    isFirstFalling = false;
  }
}
