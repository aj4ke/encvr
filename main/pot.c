#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "pot.h"

static adc_oneshot_unit_handle_t pot_handle;
static adc_channel_t pot_channel;

void pot_init(void){
    adc_unit_t unit;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(POT_GPIO_PIN, &unit, &pot_channel));

    adc_oneshot_unit_init_cfg_t pot_unit_cfg = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&pot_unit_cfg, &pot_handle));

    adc_oneshot_chan_cfg_t pot_chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(pot_handle, pot_channel, &pot_chan_cfg));
}

int pot_read_raw(void){
    int sum = 0; 
    for(int i = 0; i < 8; i++){
        int raw; 
        ESP_ERROR_CHECK(adc_oneshot_read(pot_handle, pot_channel, &raw));
        sum += raw;
    }
    return sum / 8; 
}
int pot_get_index(int n_items){
    // dividing the pot into n equal zones
    
    static int last_index = -1;
    static int last_n;
    static int low, high, margin;

    int raw = pot_read_raw();
    int index = raw * n_items / 4096;

    if(last_index == -1 || n_items != last_n){
        last_index = index;
        last_n = n_items;
        return index;
    } else {
        low = last_index * 4096 / n_items;
        high = (last_index + 1) * 4096 / n_items;
        margin = 4096 / n_items / 4; // quarter zone's width
    }

    if(raw < low - margin || raw > high + margin){
        last_index = index;
    }

    return last_index;
}