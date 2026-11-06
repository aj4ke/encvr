#include "radio.h"
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "timer.h"

#define RADIO_UART_PORT UART_NUM_1
#define RADIO_TXD_PIN   17
#define RADIO_RXD_PIN   16
#define RADIO_BAUD_RATE 115200
#define RADIO_RESET_PIN GPIO_NUM_18

typedef enum {
    ERR_MISSING_ENTER        = 1,
    ERR_MISSING_HEAD         = 2,
    ERR_UNKNOWN_CMD          = 4,
    ERR_LEN_MISMATCH         = 5,
    ERR_TX_OVER              = 10,
    ERR_CRC                  = 12,
    ERR_TX_EXCEED            = 13,
    ERR_FAIL_WRITE_FLASH     = 14,
    ERR_UNKNOWN              = 15,
    ERR_TX_INCOMPLETE        = 17,
    ERR_PREAMBLE_NOT_ALLOWED = 18,
    ERR_RX_FAIL              = 19,
    ERR_PS_FAIL              = 20
} RadioErrCode_t;

static const char *const AT_cmds[] = {
    [RADIO_STATUS_WORK_MODE]    = "AT+MODE?\r\n",
    [RADIO_STATUS_BAUD_RATE]    = "AT+IPR?\r\n",
    [RADIO_STATUS_FREQUENCY]    = "AT+BAND?\r\n",
    [RADIO_STATUS_PARAMETERS]   = "AT+PARAMETER?\r\n",
    [RADIO_STATUS_ADDRESS_ID]   = "AT+ADDRESS?\r\n",
    [RADIO_STATUS_NETWORK_ID]   = "AT+NETWORKID?\r\n",
    [RADIO_STATUS_PASSWORD]     = "AT+CPIN?\r\n",
    [RADIO_STATUS_OUTPUT_POWER] = "AT+CRFOP?\r\n",
    [RADIO_STATUS_LAST_TX_DATA] = "AT+SEND?\r\n",
    [RADIO_STATUS_MODEL_ID]     = "AT+UID?\r\n",
    [RADIO_STATUS_FIRMWARE_VER] = "AT+VER?\r\n"
};

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
    ESP_ERROR_CHECK(uart_set_pin(RADIO_UART_PORT, RADIO_TXD_PIN, RADIO_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(RADIO_UART_PORT, 256, 0, 0, NULL, 0));
}

static const char* error_check(char* reply){
    if(strncmp(reply, "+ERR=", 5) != 0){
        return "Unknown error occured.";
    }

    switch(atoi(reply + 5)) {
        case ERR_MISSING_ENTER:        return "There is not 'enter' or 0x0D 0x0A in the end of the AT Command.";
        case ERR_MISSING_HEAD:         return "The head of AT command is not “AT” string.";
        case ERR_UNKNOWN_CMD:          return "Unknown command.";
        case ERR_LEN_MISMATCH:         return "The data to be sent does not match the actual length";
        case ERR_TX_OVER:              return "TX is over times.";
        case ERR_CRC:                  return "CRC error.";
        case ERR_TX_EXCEED:            return "TX data exceeds 240bytes.";
        case ERR_FAIL_WRITE_FLASH:     return "Failed to write flash memory.";
        case ERR_UNKNOWN:              return "Unknown failure.";
        case ERR_TX_INCOMPLETE:        return "Last TX was not complete";
        case ERR_PREAMBLE_NOT_ALLOWED: return "Preamble value is not allowed.";
        case ERR_RX_FAIL:              return "RX failed, Header error.";
        case ERR_PS_FAIL:              return "The time setting value of the “Smart receiving power saving mode” is not allowed.";
        default:                       return "Unknown error code.";
    }
}

bool radio_get_status(char *buf, size_t buf_len, RadioStatusID_t cmd, const char **err) {
    // to get the prefix skip the first two chars and then append '=' to the end. 
    const char *command = AT_cmds[cmd];
    uart_write_bytes(RADIO_UART_PORT, command, strlen(command));
    char reply[64];
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)reply, sizeof(reply) - 1, pdMS_TO_TICKS(500));
    reply[len > 0 ? len : 0] = '\0';
    
    char response_prefix[16];
    size_t j = 0; 
    for(size_t i = 2; command[i] != '\0'; i++){
        if(strchr("?\r\n", command[i]) == NULL){
            response_prefix[j++] = command[i];
        }
    }
    response_prefix[j++] = '=';
    response_prefix[j] = '\0';
    
    buf[0] = '\0';

    // AT guide shows AT+NETWORKID? replying +NETWORK=
    const char *match; 
    if (cmd == RADIO_STATUS_NETWORK_ID){
        match = "+NETWORK=";
    }else{
        match = response_prefix;
    }

    char *value = strstr(reply, match);
    if (!value) {
        *err = error_check(reply);
        return false;
    }
    value += strlen(match);

    // no line ending then the reply got cut off
    char *end = strpbrk(value, "\r\n");
    if (!end) {
        *err = "Incomplete response.";
        return false;
    }
    *end = '\0';

    strncpy(buf, value, buf_len - 1);
    buf[buf_len - 1] = '\0';
    return true;
}

bool radio_set_default(const char **err){
    const char *cmd = "AT+FACTORY\r\n";

    uart_write_bytes(RADIO_UART_PORT, cmd, strlen(cmd));
    char response[18];
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)response, sizeof(response) - 1, pdMS_TO_TICKS(500));

    response[len > 0 ? len : 0] = '\0';
    const char *expected_response = "+FACTORY\r\n";
    char *resp_exists = strstr(response, expected_response);

    if (!resp_exists) {
        *err = error_check(response);
        return false;
    }
    return true;
}

bool radio_reset() {
    static char response[9]; // +READY\r\n -> 8 chars + 1 for null terminator
    gpio_config(&reset_gpio_conf);
    
    //set the timer low for 100ms
    setup_pulse_timer();
    pulse_low(RADIO_RESET_PIN, 100);

    // monitor the uart tx pin via a serial buffer
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)response, sizeof(response) - 1, pdMS_TO_TICKS(500));
    response[len > 0 ? len : 0] = '\0';
    const char *expected_response = "+READY\r\n";
    char *resp_exists = strstr(response, expected_response);
    
    // replace this with a timeout check or something 
    // reset will never return an error
    return resp_exists == NULL ? false : true;
}
