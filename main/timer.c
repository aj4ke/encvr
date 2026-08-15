#include "esp_timer.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "timer.h"

static esp_timer_handle_t pulse_timer;
static gpio_num_t pulse_pin;

static void timer_callback(void* arg) {
    gpio_set_level(pulse_pin, 1);
}

void setup_timer(void) {
    if (pulse_timer != NULL) {
        return;
    }
    const esp_timer_create_args_t args = {
        .callback = &timer_callback,
        .name = "pulse_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &pulse_timer));
}

void pulse_low(gpio_num_t pin, uint32_t duration_ms) {
    pulse_pin = pin;
    gpio_set_level(pin, 0);
    esp_timer_start_once(pulse_timer, (uint64_t)duration_ms * 1000);
}
