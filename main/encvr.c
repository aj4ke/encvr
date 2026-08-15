#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "radio.h"

void app_main(void) {
    display_init();
    radio_init();

    //char status[64];
    //radio_get_status(status, sizeof(status));

    bool success = radio_set_default();
    bool success2 = radio_reset();

    char status_text[64];
    snprintf(status_text, sizeof(status_text), "%s\n%s",
        success ? "Settings reset" : "Err",
        success2 ? "Radio reset" : "Err");
    display_text(status_text);
}
