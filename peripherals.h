#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "stdbool.h"

void init_keyboard(void);

void init_display(int width, int height);

void clear_display(void);

void draw_pixel(int x, int y, bool is_on);

void set_keys(void);

#endif
