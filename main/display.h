#pragma once

// Init the display to display text
void display_init(void);

// Shows text on the OLED, scrolling automatically if it's too wide to fit
void display_text(const char *text);

void clear_screen(void);
