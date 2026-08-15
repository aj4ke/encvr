#include "button.h"
#include "display.h"

static volatile bool btn_flag = false;

static void IRAM_ATTR btn_interrupt(void* arg) {
    btn_flag = true;
}

void button_init(void) {
    static const gpio_config_t button_gpio_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, //enable internal pullup resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE //button breaks circuit so falling edge interrupt
    };
    gpio_config(&button_gpio_conf);
    gpio_install_isr_service(ESP_INTR_CPU_AFFINITY_AUTO);
    gpio_isr_handler_add(BUTTON_GPIO_PIN, btn_interrupt, NULL);
}

bool button_was_pressed(void) {
    bool pressed = btn_flag;
    btn_flag = false;
    return pressed;
}
