idf_component_register(SRCS "display.c" "radio.c" "timer.c" "encvr.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio esp_driver_uart)
