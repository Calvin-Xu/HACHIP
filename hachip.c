#include "hachip.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "ps2.h"
#include "roms.h"
#include "strings.h"

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

#define FONT_START 0x50
void init_chip(void) {
  CHIP.OPCODE = 0;
  CHIP.PC = 0x200;
  CHIP.I = 0;
  CHIP.SP = 0;
  memset(CHIP.MEM, 0, sizeof(CHIP.MEM));
  memset(CHIP.V, 0, sizeof(CHIP.V));
  memset(CHIP.STACK, 0, sizeof(CHIP.STACK));
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    memset(CHIP.PIXELS + i, 0, DISPLAY_WIDTH);
  }
  for (int i = 0; i < 80; i++) {
    CHIP.MEM[i + FONT_START] = font[i];
  }
}

void load_program(unsigned short *program) {
  memcpy(CHIP.MEM + 0x200, &program, sizeof(program));
}

void emulate_cycle(void) {}

void draw_graphics(void);

void set_keys(void);

int main() {
  init_keyboard();
  init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  init_chip();
  load_program(IBM_LOGO);
  while (true) {
    emulate_cycle();
    set_keys();
  }
  return 0;
}
