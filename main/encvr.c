#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "display.h"
#include "radio.h"
#include "button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_main";

void vATaskFunction(void *pvParameters){
    display_init();
    radio_init();
    button_init();

    bool default_ok = radio_set_default();
    ESP_LOGI(TAG, "radio_set_default: %s", default_ok ? "ok" : "failed");

    bool reset_ok = radio_reset();
    ESP_LOGI(TAG, "radio_reset: %s", reset_ok ? "ok" : "failed");

    char band[32];
    radio_get_status(band, sizeof(band), RADIO_STATUS_FREQUENCY);
    ESP_LOGI(TAG, "radio_get_status (band): %s", band);

    char status_text[128];
    snprintf(status_text, sizeof(status_text),
        "Default: %s\nReset: %s\nBand: %s",
        default_ok ? "OK" : "Err",
        reset_ok ? "OK" : "Err",
        band[0] ? band : "n/a");
    display_text(status_text);

    for(;;){
        bool interrupt = button_was_pressed();
        if (interrupt) {
            display_text("1");
            vTaskDelay(pdMS_TO_TICKS(500));
            display_text(status_text);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    xTaskCreate(vATaskFunction, "button_task", 2048, NULL, 5, NULL);
}
