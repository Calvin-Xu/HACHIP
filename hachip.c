#include "hachip.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "printf.h"
#include "ps2.h"
#include "roms.h"
#include "strings.h"

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

void load_program(unsigned short *program, size_t size) {
  memcpy(CHIP.MEM + 0x200, program, size);
}

void emulate_cycle(void) {
  printf("Cycle start:\n");
  CHIP.OPCODE = CHIP.MEM[CHIP.PC] | CHIP.MEM[CHIP.PC + 1] << 8;
  CHIP.PC += 2;
  printf("Opecode: %04x\n", CHIP.OPCODE);
  run_opcode(CHIP.OPCODE);
  printf("Cycle end:\n");
}

// https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
void run_opcode(unsigned short opcode) {
  switch (opcode & 0xF000) {
  case 0:
    // TODO:
    // 0NNN: Execute machine language subroutine at address NNN
    // 00E0: Clear the screen
    // 00EE: Return from a subroutine
    break;
  case 0x1000:
    // TODO:
    // 1NNN: Jump to address NNN
    break;
  case 0x2000:
    // TODO:
    // 2NNN: Execute subroutine starting at address NNN
    break;
  case 0x3000:
    // TODO:
    // 3XNN: Skip the following instruction if the value of register VX equals
    // NN
    break;
  case 0x4000:
    // TODO:
    // 4XNN: Skip the following instruction if the value of register VX is not
    // equal to NN
    break;
  case 0x5000:
    // TODO:
    // 5XY0: Skip the following instruction if the value of register VX is equal
    // to the value of register VY
    break;
  case 0x6000:
    // TODO:
    // 6XNN: Store number NN in register VX
    break;
  case 0x7000:
    // TODO:
    // 7XNN: Add the value NN to register VX
    break;
  case 0x8000:
    // TODO:
    // 8XY0: Store the value of register VY in register VX
    // 8XY1: Set VX to VX OR VY
    // 8XY2: Set VX to VX AND VY
    // 8XY3: Set VX to VX XOR VY
    // 8XY4: Add the value of register VY to register VX
    //       Set VF to 01 if a carry occurs
    //       Set VF to 00 if a carry does not occur
    // 8XY5: Subtract the value of register VY from register VX
    //       Set VF to 00 if a borrow occurs
    //       Set VF to 01 if a borrow does not occur
    // 8XY6: Store the value of register VY shifted right one bit in register
    //       VX
    //       Set register VF to the least significant bit prior to the shift
    //       VY is unchanged
    // 8XY7: Set register VX to the value of VY minus VX
    //       Set VF to 00 if a borrow occurs
    //       Set VF to 01 if a borrow does not occur
    // 8XYE: Store the value of register VY shifted left one bit in register VX¹
    //       Set register VF to the most significant bit prior to the shift
    //       VY is unchanged
    break;
  case 0x9000:
    // TODO:
    // 9XY0: Skip the following instruction if the value of register VX is not
    //       equal to the value of register VY
    break;
  case 0xA000:
    // TODO:
    // ANNN: Store memory address NNN in register I
    break;
  case 0xB000:
    // TODO:
    // BNNN: Jump to address NNN + V0
    break;
  case 0xC000:
    // TODO:
    // CXNN: Set VX to a random number with a mask of NN
    break;
  case 0xD000:
    // TODO:
    // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data
    //       starting at the address stored in I
    //       Set VF to 01 if any set pixels are changed to unset, and 00
    //       otherwise
    break;
  case 0xE000:
    // TODO:
    // EX9E: Skip the following instruction if the key corresponding to the hex
    //       value currently stored in register VX is pressed
    // EXA1: Skip the following instruction if the key corresponding to the hex
    //       value currently stored in register VX is not pressed
    break;
  case 0xF000:
    // TODO:
    // FX07: Store the current value of the delay timer in register VX
    // FX0A: Wait for a keypress and store the result in register VX
    // FX15: Set the delay timer to the value of register VX
    // FX18: Set the sound timer to the value of register VX
    // FX1E: Add the value stored in register VX to register I
    // FX29: Set I to the memory address of the sprite data corresponding to the
    //       hexadecimal digit stored in register VX \
    // FX33: Store the binary-coded decimal equivalent of the value stored in
    //       register VX at addresses I, I + 1, and I + 2
    // FX55: Store the values of registers V0 to VX inclusive in memory starting
    //       at address I
    //       I is set to I + X + 1 after operation
    // FX65: Fill registers V0 to VX inclusive with the values stored in memory
    //       starting at address I
    //       I is set to I + X + 1 after operation
    break;
  }
}

void draw_graphics(void);

void set_keys(void) {}

int main() {
  init_keyboard();
  init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  init_chip();
  load_program(IBM_LOGO, sizeof(IBM_LOGO));
  while (true) {
    emulate_cycle();
    set_keys();
  }
  return 0;
}
