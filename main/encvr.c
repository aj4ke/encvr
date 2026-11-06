#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"
#include "display.h"
#include "pot.h"
#include "radio.h"

void vATaskFunction(void *pvParameters){
    display_init();
    display_show_splash();
    radio_init();
    button_init();
    pot_init();

    const char *default_err = NULL;
    const char *band_err = NULL;

    bool default_ok = radio_set_default(&default_err);
    bool reset_ok = radio_reset();
    char band[32];

    radio_get_status(band, sizeof(band), RADIO_STATUS_FREQUENCY, &band_err);

    char status_text[128];
    snprintf(status_text, sizeof(status_text), "Default: %s\nReset: %s\nBand: %s",
        default_ok ? "OK" : default_err,
        reset_ok ? "OK" : "err",
        band[0] ? band : band_err
        );

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