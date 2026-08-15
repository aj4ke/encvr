#pragma once
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_intr_types.h"

#define BUTTON_GPIO_PIN GPIO_NUM_23

void button_init(void);
bool button_was_pressed(void);