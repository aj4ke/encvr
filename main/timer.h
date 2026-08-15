#pragma once
#include "driver/gpio.h"

// Sets up pulse timer
void setup_pulse_timer(void);

// Drives pin low, then releases it back high after duration_ms
void pulse_low(gpio_num_t pin, uint32_t duration_ms);
