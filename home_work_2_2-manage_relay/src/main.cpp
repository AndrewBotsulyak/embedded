#include <Arduino.h>

#define RELAY_MANAGE_PIN 7
#define RELAY_STATE_PIN 4
#define POWER_PIN 10

#define COUNT_RESULTS 10

// VARIABLES
volatile bool isRelayOn = false;
volatile bool isFirstFalling = true;
volatile ulong isrTime = 0;
volatile ulong isrTimeMks = 0;
ulong actionTimeMks = 0;
ulong actionTime = 0;
bool isRelaySetted = false;
uint32_t count = 0;
ulong arrResult[COUNT_RESULTS];
ulong arrResultMks[COUNT_RESULTS];


// ISRs
void IRAM_ATTR handleRelayOn();

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_MANAGE_PIN, OUTPUT);
  pinMode(RELAY_STATE_PIN, INPUT_PULLDOWN);
  pinMode(POWER_PIN, OUTPUT);

  digitalWrite(RELAY_MANAGE_PIN, HIGH); // OFF
  digitalWrite(POWER_PIN, HIGH);

  Serial.printf("digitalWrite(RELAY_MANAGE_PIN, HIGH); \n");

  delay(1000);

  attachInterrupt(digitalPinToInterrupt(RELAY_STATE_PIN), handleRelayOn, FALLING);
}

void loop() {

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

        Serial.printf("%d check = %d \n", count, arrResult[count]);

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
      Serial.printf("%d - ", arrResult[i]);
    }
    Serial.printf("\n micros array ... \n");
    for (size_t i = 0; i < COUNT_RESULTS; i++) {
      Serial.printf("%d - ", arrResultMks[i]);
    }
    count++;
  }
}


void IRAM_ATTR handleRelayOn() {
  if (isFirstFalling == true) {
    isrTimeMks = micros();
    isrTime = millis();
    isRelayOn = true;
    isFirstFalling = false;
  }
}