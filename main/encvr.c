#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "radio.h"

static const char *TAG = "test";

void vATaskFunction(void *pvParameters){
    radio_init();

    const char *resp = NULL;
    const char *err = NULL;

    bool reset_ok = radio_reset();
    ESP_LOGI(TAG, "Reset: %s", reset_ok ? "OK" : "No +READY");

    err = NULL;
    radio_set_default(&err);
    ESP_LOGI(TAG, "Default: %s", err ? err : "OK");

    radio_send_cmd(&resp, &err, false, AT_TEST);
    ESP_LOGI(TAG, "Test: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, true, AT_ADDRESS, 120);
    ESP_LOGI(TAG, "Set addr: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, true, AT_NETID, 14);
    ESP_LOGI(TAG, "Set netid: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, true, AT_PARAMETER, 10, 9, 3, 12);
    ESP_LOGI(TAG, "Set param: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, true, AT_CRFOP, 14);
    ESP_LOGI(TAG, "Set power: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_ADDRESS);
    ESP_LOGI(TAG, "Addr: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_NETID); // broken, netID query returning unknown error for some reason
    ESP_LOGI(TAG, "NetID: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_PARAMETER);
    ESP_LOGI(TAG, "Param: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_CRFOP);
    ESP_LOGI(TAG, "Power: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_BAND);
    ESP_LOGI(TAG, "Band: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_UID);
    ESP_LOGI(TAG, "UID: %s", err ? err : resp);

    radio_send_cmd(&resp, &err, false, AT_VER);
    ESP_LOGI(TAG, "Ver: %s", err ? err : resp);

    vTaskDelete(NULL);
}

void app_main(void) {
    xTaskCreate(vATaskFunction, "radio_test", 4096, NULL, 5, NULL);
}
