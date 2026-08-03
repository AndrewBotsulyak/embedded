#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"


#include "isr-button/isr-button.h"
#include "blink/blink.h"

void app_main(void)
{
    gpio_install_isr_service(0);

    DebouncedBtn btn1 = getDefaultBtn(17);
    isrButtonInit(&btn1);
    blink_setup();

    while (1) {
        handleDebounce(&btn1);

        if (btn1.isPressed) {
            blink_task();
        }
        else if(btn1.isPressed == false){
            blink_reset();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
