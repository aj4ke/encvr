#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "radio.h"
#include "button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pot.h"

void vATaskFunction(void *pvParameters){
    display_init();
    display_show_splash();
    radio_init();
    button_init();
    pot_init();

    bool default_ok = radio_set_default();
    bool reset_ok = radio_reset();

    char band[32];
    radio_get_status(band, sizeof(band), RADIO_STATUS_FREQUENCY);

    char status_text[128];
    snprintf(status_text, sizeof(status_text),
        "Default: %s\nReset: %s\nBand: %s",
        default_ok ? "OK" : "Err",
        reset_ok ? "OK" : "Err",
        band[0] ? band : "n/a");

    char screen_text[sizeof(status_text) + 16];
    int last_index = -1;
    display_hide_splash();

    for(;;){
        bool interrupt = button_press();
        if (interrupt) {
            display_text("1");
            vTaskDelay(pdMS_TO_TICKS(500));
            last_index = -1;
        }

        int index = pot_get_index(5);
        if (index != last_index) {
            snprintf(screen_text, sizeof(screen_text), "%s\nPot: %d", status_text, index);
            display_text(screen_text);
            last_index = index;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    xTaskCreate(vATaskFunction, "button_task", 4096, NULL, 5, NULL);
}