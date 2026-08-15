#include "radio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "timer.h"
#include <stdlib.h>
#include <string.h>

static const gpio_config_t reset_gpio_conf = {
    .pin_bit_mask = (1ULL << RADIO_RESET_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};

void radio_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = RADIO_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_param_config(RADIO_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(RADIO_UART_PORT, RADIO_TXD_PIN, RADIO_RXD_PIN,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(RADIO_UART_PORT, 256, 0, 0, NULL, 0));
}

void radio_get_status(char *buf, size_t buf_len, RadioStatusID_t status) {
    //TODO: Add support for the other status read IDs
    const char *cmd = "AT+BAND?\r\n";
    uart_write_bytes(RADIO_UART_PORT, cmd, strlen(cmd));
    char reply[64];
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)reply, sizeof(reply) - 1, pdMS_TO_TICKS(500));
    reply[len > 0 ? len : 0] = '\0';
    const char *prefix = "+BAND=";
    char *band = strstr(reply, prefix);
    if (band) {
        band += strlen(prefix);
        char *end = strpbrk(band, "\r\n");
        if (end) {
            *end = '\0';
        }
    }
    strncpy(buf, band ? band : "", buf_len - 1);
    buf[buf_len - 1] = '\0';
}

bool radio_set_default(void){
    const char* TAG = "__response__";
    const char *cmd = "AT+FACTORY\r\n";

    uart_write_bytes(RADIO_UART_PORT, cmd, strlen(cmd));
    char response[18];
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)response, sizeof(response) - 1, pdMS_TO_TICKS(500));

    response[len > 0 ? len : 0] = '\0';
    const char *expected_response = "+FACTORY\r\n";
    ESP_LOGI(TAG, "%s : %d\n,",response, len);
    char *resp_exists = strstr(response, expected_response);

    return resp_exists == NULL ? false : true;
}

bool radio_reset(void) {
    const char* TAG = "__response__";
    static char response[9]; // +READY\r\n -> 8 chars + 1 for null terminator
    gpio_config(&reset_gpio_conf);
    
    //set the timer low for 100ms
    setup_pulse_timer();
    pulse_low(RADIO_RESET_PIN, 100);

    // monitor the uart tx pin via a serial buffer
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)response, sizeof(response) - 1, pdMS_TO_TICKS(500));
    response[len > 0 ? len : 0] = '\0';
    const char *expected_response = "+READY\r\n";
    ESP_LOGI(TAG, "%s : %d\n,",response, len);
    char *resp_exists = strstr(response, expected_response);
    
    return resp_exists == NULL ? false : true;
}


// TODO: make helper function for reading the uart buffer so I don't gotta repeat ts logic 