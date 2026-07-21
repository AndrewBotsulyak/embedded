#include <Arduino.h>
#include <math.h>

#define ADC_PIN 4
#define LDR_PIN 7
#define LED_PIN 10
#define VDD 3.3
#define INCREASE_BASE_LED_LIGH_PIN 5
#define DECREASE_BASE_LED_LIGH_PIN 6
#define DEBOUNCE_TIME 20
// 
uint8_t channel = 0;
uint32_t freq = 5000;
uint8_t resolution = 12;
uint16_t adcRange = pow(2, resolution); // 4096
uint8_t tresholdPercent = 65;
float_t tresholdStep = (float_t)adcRange / 100.f;

float_t vddVoltageStep = VDD / adcRange; // for 3.3V = 0.000805 V

uint16_t pwmRange = pow(2, resolution); // 4096
float_t pwmRangeStep = (pwmRange / 100.f);
float_t maxRawValue = pwmRangeStep * 100; // граница raw = 100% от resolution (4096)
float_t minRawHysteresis = pwmRangeStep * 65; // turn led from 65% of dark
float_t maxRawHysteresis = pwmRangeStep * 60; // turn off only if low than 60% of dark
float_t rawStep = (maxRawValue - minRawHysteresis) / 100; // вычисляем 1% рабочего диапазона raw
volatile ulong currentTime = 0;


// LED variables
float_t minDutyLedLight;
uint16_t maxDutyLedLight;
float_t workPwmRangeStep;
uint8_t maxDutyLight = 100; // max %
uint8_t minDutyLight = 1; // %
uint8_t isrIncreasedLight = 0;
const uint8_t isrIncreasedLightStep = 10;
volatile bool increaseLightRequested = false;
volatile bool decreaseLightRequested = false;

typedef enum {
  INCREASE_BASE_LIGHT,
  DECREASE_BASE_LIGHT
} ButtonTypes;

typedef struct {
  volatile ulong lastTimeChecked;
  volatile bool isDebouncing;
  volatile bool isrRequested;
  ButtonTypes type;
} DebouncedBtn;

volatile DebouncedBtn increaseBtn = {
  .lastTimeChecked = 0,
  .isDebouncing = false,
  .isrRequested = false,
  .type = INCREASE_BASE_LIGHT
};

volatile DebouncedBtn decreaseBtn = {
  .lastTimeChecked = 0,
  .isDebouncing = false,
  .isrRequested = false,
  .type = DECREASE_BASE_LIGHT
};

// FUNCTIONS
uint32_t getLedDuty(uint16_t raw);
void ledLightSetup(uint8_t x);
void increaseBaseLight();
void decreaseBaseLight();
void IRAM_ATTR increaseBaseLightISR();
void IRAM_ATTR decreaseBaseLightISR();
void handleDebounce(volatile DebouncedBtn* btn, uint8_t pin);


void setup() {
  Serial.begin(115200);

  ledLightSetup(minDutyLight);

  // optional functions
  analogReadResolution(resolution);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  pinMode(LDR_PIN, OUTPUT);
  pinMode(INCREASE_BASE_LED_LIGH_PIN, INPUT_PULLUP);
  pinMode(DECREASE_BASE_LED_LIGH_PIN, INPUT_PULLUP);
  
  

  // Interupts
  attachInterrupt(digitalPinToInterrupt(INCREASE_BASE_LED_LIGH_PIN), increaseBaseLightISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DECREASE_BASE_LED_LIGH_PIN), decreaseBaseLightISR, CHANGE);

  ledcSetup(channel, freq, resolution);
  ledcAttachPin(LED_PIN, channel);

  digitalWrite(LDR_PIN, HIGH);


  Serial.printf("raw >= maxRawValue = %d \n", 4 >> 3);
}

void loop() {

  // interrupt flag
  // increase minimum base light button clicked
  if (increaseBtn.isDebouncing == true) {
    handleDebounce(&increaseBtn, INCREASE_BASE_LED_LIGH_PIN);
  }

  // interrupt flag
  // decrease minimum base light button clicked
  if (decreaseBtn.isDebouncing == true) {
    handleDebounce(&decreaseBtn, DECREASE_BASE_LED_LIGH_PIN);
  }

  uint16_t raw = analogRead(ADC_PIN);
  float_t calcVoltage = vddVoltageStep * raw;

  float_t volts = analogReadMilliVolts(ADC_PIN) / 1000.0f;
  float_t percentage = raw / tresholdStep;

  uint32_t ledDuty = getLedDuty(raw);

  // Serial.printf("ledDuty = %d \n", ledDuty);
  ledcWrite(channel, ledDuty);

  // Serial.printf("raw = %d, calcVoltage = %2f V, volts = %2f V \n", raw, calcVoltage, volts);
  // Serial.printf("difference = %2f V\n", volts - calcVoltage);
  
  delay(300);
}

uint32_t getLedDuty(const uint16_t raw) {
  uint32_t retVal = 0;

  // если raw больше чем максимальное значение которое можно повторить для LDR += 4000
  if (raw >= maxRawValue) {
    // ставим максимальное значение / яркость
    retVal = maxDutyLedLight;

    // Serial.printf("raw >= maxRawValue = %d \n", retVal);
  }
  // если больше чем минимум
  else if (raw > minRawHysteresis) {

    // узнаем какой процент относительно рабочего диапазона (minRawValue - maxRawValue)
    // и конвертируем эти проценты относительно всего диапазона
    // например raw = 4000
    // (4000 - 2600) / rawStep) = 93% 
    // 93 * 30.96 = 1548 + 1000 = 2548
    retVal = ((raw - minRawHysteresis) / rawStep) * workPwmRangeStep + minDutyLedLight;

    // Serial.printf("raw > minRawValue = %d \n", retVal);
  }
  else if (raw > maxRawHysteresis) {
    retVal = minDutyLedLight;
  }

  
  return retVal;
}

/*
  minDutyLight - минимальная яркость, проценты относительно всего диапазона PWM
*/
void ledLightSetup(const uint8_t minDutyLight) {
  minDutyLedLight = pwmRangeStep * minDutyLight; // min скважность 1% по дефолту
  maxDutyLight = 100;
  maxDutyLedLight = pwmRangeStep * maxDutyLight;
  workPwmRangeStep = (maxDutyLedLight - minDutyLedLight) / 100;
}

void IRAM_ATTR increaseBaseLightISR() {
  increaseBtn.isrRequested = true;
  increaseBtn.isDebouncing = true;
}

void IRAM_ATTR decreaseBaseLightISR() {
  decreaseBtn.isrRequested = true;
  decreaseBtn.isDebouncing = true;
}

void handleDebounce(volatile DebouncedBtn* btn, uint8_t pin) {
  // если все еще в процесе устранения дребезга
  if (btn->isrRequested == true) {
    btn->lastTimeChecked = millis();
    btn->isrRequested = false;
  }
  // если начали отсчет и кнопка нажата
  else if(btn->isrRequested == false && digitalRead(pin) == LOW) {
    ulong now = millis();

    if (now - btn->lastTimeChecked > DEBOUNCE_TIME) {

      switch (btn->type)
      {
      case INCREASE_BASE_LIGHT:
        increaseBaseLight();
        break;
      case DECREASE_BASE_LIGHT:
        decreaseBaseLight();
      break;

      default:
        break;
      }
      
      Serial.printf("btn->type = %d \n", btn->type);
      btn->isDebouncing = false;
    }
  }
  else if (btn->isrRequested == false && digitalRead(pin) == HIGH) {
    ulong now = millis();
    if (now - btn->lastTimeChecked > DEBOUNCE_TIME) {
      Serial.printf("reset isDebouncing when HIGH \n");
      btn->isDebouncing = false;
    }
  }
}

void increaseBaseLight() {
  increaseLightRequested = false;
  // считаю реальное желаемое значение
  uint16_t requestedPercent = isrIncreasedLight + isrIncreasedLightStep + minDutyLight;

  // если мы хотим увеличить яркость которое превышает допустимое - пропускает
  if (requestedPercent <= maxDutyLight) {
    // увеличиваем базовую яркость по нажатию кнопки
    isrIncreasedLight += isrIncreasedLightStep;
    // пересчитываем управляющие переменные
    ledLightSetup(requestedPercent);
  }
}

void decreaseBaseLight() {
  // считаю реальное желаемое значение
  int16_t requestedPercent = isrIncreasedLight - isrIncreasedLightStep + minDutyLight;

  // если мы хотим увеличить яркость которое превышает допустимое - пропускает
  if (requestedPercent >= minDutyLight) {
    // увеличиваем базовую яркость по нажатию кнопки
    isrIncreasedLight -= isrIncreasedLightStep;
    // пересчитываем управляющие переменные
    ledLightSetup(requestedPercent);
  }

}