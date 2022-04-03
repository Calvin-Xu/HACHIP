#include "hachip.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "ps2.h"
#include "roms.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

void init_keyboard(void) {
  gpio_init();
  keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
  // https://wiki.osdev.org/%228042%22_PS/2_Controller#PS.2F2_Controller_Commands
  ps2_device_t *keyboard = ps2_new(KEYBOARD_CLOCK, KEYBOARD_DATA);
  while (true) {
    if (keyboard_read_scancode() == 0x55) {
      ps2_write(keyboard, 0x55);
      break;
    }
  }
  free(keyboard);
  interrupts_init();
  keyboard_use_interrupts();
  interrupts_global_enable();
}

void init_display(unsigned int width, unsigned int height) {
  gl_init(width, height, GL_DOUBLEBUFFER);
  gl_clear(GL_BLACK);
}

void init_chip(void);

void load_program(unsigned short *);

void emulate_cycle(void);

void draw_graphics(void);

void set_keys(void);

int main() {
  init_keyboard();
  init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  load_program(TEST_ROM);
  while (true) {
    emulate_cycle();
    set_keys();
  }
  return 0;
}
