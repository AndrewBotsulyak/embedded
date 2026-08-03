#include <stdint.h>
#include "driver/gpio.h"

typedef struct {
    int64_t lastTimeChecked;
    volatile bool isDebouncing;
    volatile bool isrRequested;
    gpio_num_t pin;
    bool isPressed;
} DebouncedBtn;

void isrButtonInit(DebouncedBtn* btn);
void handleDebounce(DebouncedBtn* btn);
DebouncedBtn getDefaultBtn(gpio_num_t pinNumber);