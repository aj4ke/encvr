#include "display.h"    
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ssd1306.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

static lv_obj_t *message_label;

static void set_label_y(void *label, int32_t y) {
    lv_obj_set_y((lv_obj_t *)label, (lv_coord_t)y);
}

void display_init(void) {
    // Sets up the I2C bus the OLED is wired to
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // Wraps the I2C bus in the panel IO interface
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_I2C_ADDR,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
        .scl_speed_hz = 400000,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

    // Configures the SSD1306 panel driver
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_HEIGHT,
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = &ssd1306_config,
    };
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    // Powers on the panel and readies it for drawing
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Starts LVGL tick timer, refresh task, and locking
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // Registers the panel with LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = OLED_WIDTH * OLED_HEIGHT,
        .double_buffer = false,
        .hres = OLED_WIDTH,
        .vres = OLED_HEIGHT,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
    };
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    lvgl_port_lock(0);
    message_label = lv_label_create(lv_disp_get_scr_act(disp));
    lv_obj_align(message_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lvgl_port_unlock();
}

void display_text(const char *text) {
    lvgl_port_lock(0);
    lv_anim_del(message_label, NULL);
    lv_obj_set_pos(message_label, 0, 0);
    lv_obj_set_width(message_label, OLED_WIDTH);
    lv_obj_set_height(message_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(message_label, text);
    lv_obj_update_layout(message_label);

    lv_coord_t overflow = lv_obj_get_height(message_label) - OLED_HEIGHT;
    if (overflow > 0) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, message_label);
        lv_anim_set_exec_cb(&anim, set_label_y);
        lv_anim_set_values(&anim, 0, -overflow);
        lv_anim_set_time(&anim, overflow * 60);
        lv_anim_set_playback_time(&anim, 0);
        lv_anim_set_playback_delay(&anim, 1000);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&anim, 1000);
        lv_anim_start(&anim);
    }
    lvgl_port_unlock();
}

void clear_screen(void) {
    lvgl_port_lock(0);
    lv_anim_del(message_label, NULL);
    lv_obj_set_pos(message_label, 0, 0);
    lv_label_set_text(message_label, "");
    lvgl_port_unlock();
}
