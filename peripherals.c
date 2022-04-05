#include "peripherals.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "ps2.h"

#define PHYSICAL_WIDTH 64
#define PHYSICAL_HEIGHT 32

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

void init_display(unsigned int width, unsigned int height) {
  gl_init(width, height, GL_DOUBLEBUFFER);
  gl_clear(GL_BLACK);
}
