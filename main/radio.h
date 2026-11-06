#pragma once
#include <stddef.h>
#include "driver/gpio.h"

#define RADIO_UART_PORT UART_NUM_1
#define RADIO_TXD_PIN   17
#define RADIO_RXD_PIN   16
#define RADIO_BAUD_RATE 115200
#define RADIO_RESET_PIN GPIO_NUM_18

typedef enum {
    RADIO_STATUS_WORK_MODE = 0,
    RADIO_STATUS_BAUD_RATE,
    RADIO_STATUS_FREQUENCY,
    RADIO_STATUS_PARAMETERS,
    RADIO_STATUS_ADDRESS_ID,
    RADIO_STATUS_NETWORK_ID,
    RADIO_STATUS_PASSWORD,
    RADIO_STATUS_OUTPUT_POWER,
    RADIO_STATUS_LAST_TX_DATA,
    RADIO_STATUS_MODEL_ID,
    RADIO_STATUS_FIRMWARE_VER
} RadioStatusID_t;

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

typedef struct {
    uint16_t address; //address of sender
    int16_t RSSI; //RSSI of recieved message
    int16_t SNR; //SNR of received message 
    uint8_t length; //length of message
    char msg_buff[64]; //message buffer - only supporting 64byte long messages
} msg_response_t; 

// Brings up UART for the RYLR998
void radio_init(void);

// Queries the radio for the current value of a setting
bool radio_get_status(char *buf, size_t buf_len, RadioStatusID_t status, const char **err);

// Sets a specific setting based on the AT command passed 
//bool radio_set_setting(char *buf, size_t buf_len, RadioCmdID_t command);

// Sets all settings to default on the RYLR998 returns bool for success 
bool radio_set_default(const char **err);

// Resets the RYLR998 via its reset GPIO pin returns bool for success
bool radio_reset(void);

// Sets the ADDRESS ID of the radio 
bool radio_set_address(uint16_t addr); //ex) AT+ADDRESS=120 

// Sets the NETWORKID of the radio
bool radio_set_networkID(uint8_t networkID); //ex) AT+NETWORKID=6

// Sends data in ASCII to the address
bool radio_send_data(uint16_t addr, uint8_t len, char* msg); //ex) AT+SEND=50,5,HELLO
// check last transmit data w/ AT+SEND? 

// check for a received message, takes a pointer to the received msg struct 
void radio_check_received(msg_response_t *received_msg); 
