#pragma once

// Brings up the potentiometer, configs unit and channel
void pot_init(void); 

// Reads the raw output of the pot 0-4096
int pot_read_raw(void);

// Get's the index of the pot, divided into n sections
int pot_get_index(int n_items);