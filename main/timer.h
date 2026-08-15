#pragma once
#include "driver/gpio.h"

// Sets up the one-shot pulse timer. Call once before using pulse_low().
void setup_timer(void);

// Drives `pin` low, then releases it back high after duration_ms milliseconds.
void pulse_low(gpio_num_t pin, uint32_t duration_ms);
