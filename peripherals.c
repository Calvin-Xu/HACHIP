#include "peripherals.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "ps2.h"

#define PHYSICAL_WIDTH 800
#define PHYSICAL_HEIGHT 480

int k_display_width;
int k_display_height;

void init_keyboard(void) {
  gpio_init();
  keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
  interrupts_init();
  keyboard_use_interrupts();
  interrupts_global_enable();
  // https://wiki.osdev.org/%228042%22_PS/2_Controller#PS.2F2_Controller_Commands
  ps2_device_t *keyboard = ps2_new(KEYBOARD_CLOCK, KEYBOARD_DATA);
  while (true) {
    if (keyboard_read_scancode() == 0xAA) {
      ps2_write(keyboard, 0x55);
      break;
    }
  }
  free(keyboard);
}

void init_display(int width, int height) {
  k_display_width = width;
  k_display_height = height;
  gl_init(PHYSICAL_WIDTH, PHYSICAL_HEIGHT, GL_SINGLEBUFFER);
  gl_clear(GL_BLACK);
}

void clear_display(void) { gl_clear(GL_BLACK); }

void draw_pixel(int x, int y, bool is_on) {
  int k_scale = PHYSICAL_WIDTH / k_display_width;
  gl_draw_rect(x * k_scale, y * k_scale, k_scale, k_scale,
               is_on ? GL_WHITE : GL_BLACK);
}

void set_keys(void) {}
