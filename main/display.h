#pragma once

// Brings up the OLED and readies it for display_text()
void display_init(void);

// Shows text on the OLED, scrolling automatically if it's too wide to fit
void display_text(const char *text);
