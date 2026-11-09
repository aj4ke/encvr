#include "radio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

typedef struct {
    const char *name;
    const char *set_fmt;
} at_cmd_info_t;

static const at_cmd_info_t at_cmds[RADIO_CMD_COUNT] = {
#define X(id, name, fmt) [id] = { name, fmt },
    RADIO_CMD_T(X)
#undef X
};

#define RADIO_ERR_T(X)                                                     \
    X(ERR_MISSING_ENTER,        1,  "Missing \\r\\n at end of command.")   \
    X(ERR_MISSING_HEAD,         2,  "Command does not start with AT.")     \
    X(ERR_UNKNOWN_CMD,          4,  "Unknown command.")                    \
    X(ERR_LEN_MISMATCH,         5,  "Data length does not match.")         \
    X(ERR_TX_OVER,              10, "TX is over times.")                   \
    X(ERR_CRC,                  12, "CRC error.")                          \
    X(ERR_TX_EXCEED,            13, "TX data exceeds 240 bytes.")          \
    X(ERR_FAIL_WRITE_FLASH,     14, "Failed to write flash memory.")       \
    X(ERR_UNKNOWN,              15, "Unknown failure.")                    \
    X(ERR_TX_INCOMPLETE,        17, "Last TX was not complete.")           \
    X(ERR_PREAMBLE_NOT_ALLOWED, 18, "Preamble value is not allowed.")      \
    X(ERR_RX_FAIL,              19, "RX failed, header error.")            \
    X(ERR_PS_FAIL,              20, "Power saving time value not allowed.")

typedef enum {
#define X(name, code, msg) name = code,
    RADIO_ERR_T(X)
#undef X
} RadioErrCode_t;

static const char *radio_err_str(int code)
{
    switch (code) {
#define X(name, num, msg) case num: return msg;
    RADIO_ERR_T(X)
#undef X
    default: return "Unknown error code.";
    }
}

static const char *error_check(const char *reply)
{
    if (strncmp(reply, "+ERR=", 5) != 0) return "Unknown error occurred.";
    return radio_err_str(atoi(reply + 5));
}

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

void radio_send_cmd(const char **resp, const char **err, bool args, at_cmd_t cmd, ...) {
    static char reply[MAX_PAYLOAD_LEN + 32];
    char command[MAX_PAYLOAD_LEN + 32];
    char prefix[16];
    const at_cmd_info_t *info = &at_cmds[cmd];
    bool expect_ok = args || info->name[0] == '\0';
    int len;

    *resp = NULL;
    *err = NULL;

    if (info->name[0] == '\0') {
        len = snprintf(command, sizeof(command), "AT\r\n");
    } else if (args) {
        if (!info->set_fmt) {
            *err = "Command takes no arguments.";
            return;
        }
        len = snprintf(command, sizeof(command), "AT+%s=", info->name);

        va_list args;
        va_start(args, cmd);
        int n = vsnprintf(command + len, sizeof(command) - len, info->set_fmt, args);
        va_end(args);

        if (n < 0 || len + n + 2 > (int)sizeof(command)) {
            *err = "Command too long.";
            return;
        }
        len += n;
        command[len++] = '\r';
        command[len++] = '\n';
    } else {
        len = snprintf(command, sizeof(command), "AT+%s?\r\n", info->name);
    }

    if (expect_ok) {
        snprintf(prefix, sizeof(prefix), "+OK");
    } else if (cmd == AT_NETID) {
        // AT guide shows AT+NETWORKID? replying +NETWORK=
        snprintf(prefix, sizeof(prefix), "+NETWORK=");
    } else {
        snprintf(prefix, sizeof(prefix), "+%s=", info->name);
    }

    uart_write_bytes(RADIO_UART_PORT, command, len);
    int rx_len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)reply, sizeof(reply) - 1, pdMS_TO_TICKS(500));
    reply[rx_len > 0 ? rx_len : 0] = '\0';

    char *value = strstr(reply, prefix);
    if (!value) {
        *err = error_check(reply);
        return;
    }
    if (!expect_ok) {
        value += strlen(prefix);
    }

    // no line ending then the reply got cut off
    char *end = strpbrk(value, "\r\n");
    if (!end) {
        *err = "Incomplete response.";
        return;
    }
    *end = '\0';
    *resp = value;
}

void radio_set_default(const char **err){
    const char *cmd = "AT+FACTORY\r\n";

    uart_write_bytes(RADIO_UART_PORT, cmd, strlen(cmd));
    char response[18];
    int len = uart_read_bytes(RADIO_UART_PORT, (uint8_t *)response, sizeof(response) - 1, pdMS_TO_TICKS(500));

    response[len > 0 ? len : 0] = '\0';
    const char *expected_response = "+FACTORY\r\n";
    char *resp_exists = strstr(response, expected_response);

    if (!resp_exists) {
        *err = error_check(response);
    }
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
