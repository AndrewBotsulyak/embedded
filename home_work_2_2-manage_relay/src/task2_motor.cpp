#include <Arduino.h>
#include <task2_motor.h>


#define ADC_READ_PIN 10
#define MANAGE_PIN 7
#define POWER_PIN 4

// диапазон, который реально выдаёт потенциометр (замерить и подставить)
constexpr long rawMin = 0;
constexpr long rawMax = 4095;

// период ШИМ — независимая величина, задаёт частоту: 20 мс = 50 Гц
constexpr long pwmPeriod = 20;

void setupTask2() {
    
    pinMode(POWER_PIN, OUTPUT);
    pinMode(MANAGE_PIN, OUTPUT);
    digitalWrite(MANAGE_PIN, HIGH);

    digitalWrite(POWER_PIN, LOW); // стартуем с выключенной нагрузкой

}

void runTask2() {
    uint16_t rawADC = analogRead(ADC_READ_PIN);

    // печатаем раз в полсекунды, а не каждый период — иначе вывод в Serial
    // сам занимает заметную часть периода и искажает скважность
    static ulong lastPrint = 0;
    if (millis() - lastPrint >= 500) {
        lastPrint = millis();
        Serial.printf("rawADC = %u \n", rawADC);
    }

    // constrain обязателен: map() не ограничивает результат, а отрицательный
    // аргумент delay() превращается в uint32_t и вешает плату на ~49 суток
    long tOn = constrain(map(rawADC, rawMin, rawMax, 0, pwmPeriod), 0, pwmPeriod);
    long tOff = pwmPeriod - tOn;

    if (tOn > 0) {
        digitalWrite(POWER_PIN, HIGH);
        delay(tOn);
    }
    if (tOff > 0) {
        digitalWrite(POWER_PIN, LOW);
        delay(tOff);
    }
}