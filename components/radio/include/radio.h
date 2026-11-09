#pragma once
#include <stddef.h>
#include <stdint.h>
#define MAX_PAYLOAD_LEN 240

#define RADIO_CMD_T(X)                          \
    X(AT_TEST,      "",          NULL)          \
    X(AT_MODE,      "MODE",      "%u,%u,%u")    \
    X(AT_BAUD,      "IPR",       "%u")          \
    X(AT_BAND,      "BAND",      "%u,%u")       \
    X(AT_PARAMETER, "PARAMETER", "%u,%u,%u,%u") \
    X(AT_ADDRESS,   "ADDRESS",   "%u")          \
    X(AT_NETID,     "NETWORKID", "%u")          \
    X(AT_CPIN,      "CPIN",      "%s")          \
    X(AT_CRFOP,     "CRFOP",     "%u")          \
    X(AT_SEND,      "SEND",      "%u,%u,%s")    \
    X(AT_UID,       "UID",       NULL)          \
    X(AT_VER,       "VER",       NULL)

typedef enum {
#define X(id, name, fmt) id,
    RADIO_CMD_T(X)
#undef X
    RADIO_CMD_COUNT
} at_cmd_t;

typedef struct {
    uint16_t address; //address of sender
    int16_t RSSI; //RSSI of recieved message
    int16_t SNR; //SNR of received message 
    uint8_t length; //length of message
    char msg_buff[240]; //message buffer
} msg_response_t; 

typedef struct {
    uint16_t address;
    uint8_t payload_len;
    char data[MAX_PAYLOAD_LEN];
} radio_transmit_msg_t;

// Brings up UART for the RYLR998
void radio_init(void);

// Sends a command to the radio storing the response in resp, bool args determines if query or cmd w/ args 
void radio_send_cmd(const char **resp, const char **err, bool args, at_cmd_t cmd, ...);

// Sets all settings to default on the RYLR998 returns bool for success 
void radio_set_default(const char **err);

// Resets the RYLR998 via its reset GPIO pin returns bool for success
bool radio_reset(void);

