#include "hachip.h"
#include "assert.h"
#include "peripherals.h"
#include "printf.h"
#include "roms.h"
#include "strings.h"
#include "timer.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#define SHIFT_LEGACY_BEHAVIOR false
#define BNNN_LEGACY_BEHAVIOR false
#define STR_LDR_LEGACY_BEHAVIOR false

// #define DEBUG true

#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif

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
    memset(CHIP.PIXELS + i, false, DISPLAY_WIDTH);
  }
  memset(CHIP.KEYPAD, false, 16);
  for (int i = 0; i < 80; i++) {
    CHIP.MEM[i + FONT_START] = font[i];
  }
}

void load_program(unsigned short *program, size_t size) {
  // roms are currently stored as array of unsigned short
  // CHIP-8 is big endian; convert when loading
  for (int i = 0; i < size; i++) {
    unsigned short val = *(program + i);
    *(program + i) = (val >> 8) | (val << 8);
  }
  memcpy(CHIP.MEM + 0x200, program, size);
}

void emulate_cycle(void) {
  DEBUG_PRINT(("---Cycle start:---\n"));
  CHIP.OPCODE = CHIP.MEM[CHIP.PC] << 8 | CHIP.MEM[CHIP.PC + 1];
  CHIP.PC += 2;
  run_opcode();
  DEBUG_PRINT(("Opcode: %04x\n", CHIP.OPCODE));
  DEBUG_PRINT(("PC: %d\nI:%d\n", CHIP.PC, CHIP.I));
  for (int i = 0; i < 16; i++) {
    DEBUG_PRINT(("V[%x]: %d\n", i, CHIP.V[i]));
  }
}

#define X ((CHIP.OPCODE & 0x0F00) >> 8)
#define Y ((CHIP.OPCODE & 0x00FF) >> 4)
#define NN (CHIP.OPCODE & 0x00FF)
#define NNN (CHIP.OPCODE & 0x0FFF)
// https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
void run_opcode() {
  switch (CHIP.OPCODE & 0xF000) {
  case 0:
    if (CHIP.OPCODE & 0x0F00) {
      // 0NNN: Execute machine language subroutine at address NNN
      // Unimplemented
      DEBUG_PRINT(("Executed 0NNN\n"));
      break;
    } else {
      if (CHIP.OPCODE & 0x000F) {
        // 00EE: Return from a subroutine
        assert(CHIP.SP > 0);
        CHIP.PC = CHIP.STACK[--CHIP.SP];
        DEBUG_PRINT(("Executed 00EE\n"));
      } else {
        // 00E0: Clear the screen
        for (int i = 0; i < DISPLAY_HEIGHT; i++) {
          memset(CHIP.PIXELS + i, false, DISPLAY_WIDTH);
        }
        clear_display();
        DEBUG_PRINT(("Executed 00E0\n"));
      }
    }
    break;
  case 0x1000:
    // 1NNN: Jump to address NNN
    CHIP.PC = NNN;
    DEBUG_PRINT(("Executed 1NNN\n"));
    break;
  case 0x2000:
    // 2NNN: Execute subroutine starting at address NNN
    assert(CHIP.SP < 15);
    CHIP.STACK[CHIP.SP++] = CHIP.PC;
    CHIP.PC = NNN;
    DEBUG_PRINT(("Executed 2NNN\n"));
    break;
  case 0x3000:
    // 3XNN: Skip the following instruction if the value of register VX equals
    // NN
    if (CHIP.V[X] == NN) {
      CHIP.PC += 2;
    }
    DEBUG_PRINT(("Executed 3XNN\n"));
    break;
  case 0x4000:
    // 4XNN: Skip the following instruction if the value of register VX is not
    // equal to NN
    if (CHIP.V[X] != NN) {
      CHIP.PC += 2;
    }
    DEBUG_PRINT(("Executed 4XNN\n"));
    break;
  case 0x5000:
    // 5XY0: Skip the following instruction if the value of register VX is equal
    // to the value of register VY
    if (CHIP.V[X] == CHIP.V[Y]) {
      CHIP.PC += 2;
    }
    DEBUG_PRINT(("Executed 5XY0\n"));
    break;
  case 0x6000:
    // 6XNN: Store number NN in register VX
    CHIP.V[X] = NN;
    DEBUG_PRINT(("Executed 6XNN\n"));
    break;
  case 0x7000:
    // 7XNN: Add the value NN to register VX
    // does not set carry flag
    CHIP.V[X] += NN;
    DEBUG_PRINT(("Executed 7XNN\n"));
    break;
  case 0x8000:
    switch (CHIP.OPCODE & 0x000F) {
    case 0:
      // 8XY0: Store the value of register VY in register VX
      CHIP.V[X] = CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY0\n"));
      break;
    case 1:
      // 8XY1: Set VX to VX OR VY
      CHIP.V[X] |= CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY1\n"));
      break;
    case 2:
      // 8XY2: Set VX to VX AND VY
      CHIP.V[X] &= CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY2\n"));
      break;
    case 3:
      // 8XY3: Set VX to VX XOR VY
      CHIP.V[X] ^= CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY3\n"));
      break;
    case 4:
      // 8XY4: Add the value of register VY to register VX
      //       Set VF to 01 if a carry occurs
      //       Set VF to 00 if a carry does not occur
      CHIP.V[0XF] = CHIP.V[Y] > (0xFF - CHIP.V[X]) ? 1 : 0;
      CHIP.V[X] += CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY4\n"));
      break;
    case 5:
      // 8XY5: Subtract the value of register VY from register VX
      //       Set VF to 00 if a borrow occurs
      //       Set VF to 01 if a borrow does not occur
      CHIP.V[0xF] = CHIP.V[X] > CHIP.V[Y] ? 1 : 0;
      CHIP.V[X] -= CHIP.V[Y];
      DEBUG_PRINT(("Executed 8XY5\n"));
      break;
    case 6:
      // 8XY6: Store the value of register VY shifted right one bit in register
      //       VX
      //       Set register VF to the least significant bit prior to the shift
      //       VY is unchanged
      if (SHIFT_LEGACY_BEHAVIOR) {
        CHIP.V[X] = CHIP.V[Y];
      }
      CHIP.V[0xF] = CHIP.V[X] & 1;
      CHIP.V[X] >>= 1;
      DEBUG_PRINT(("Executed 8XY6\n"));
      break;
    case 7:
      // 8XY7: Set register VX to the value of VY minus VX
      //       Set VF to 00 if a borrow occurs
      //       Set VF to 01 if a borrow does not occur
      CHIP.V[0xF] = CHIP.V[Y] > CHIP.V[X] ? 1 : 0;
      CHIP.V[X] = CHIP.V[Y] - CHIP.V[X];
      DEBUG_PRINT(("Executed 8XY7\n"));
      break;
    case 0xE:
      // 8XYE: Store the value of register VY shifted left one bit in register
      // VX¹
      //       Set register VF to the most significant bit prior to the shift
      //       VY is unchanged
      if (SHIFT_LEGACY_BEHAVIOR) {
        CHIP.V[X] = CHIP.V[Y];
      }
      CHIP.V[0xF] = (CHIP.V[X] & 0x80) >> 7;
      CHIP.V[X] <<= 1;
      DEBUG_PRINT(("Executed 8XYE\n"));
      break;
    }
    break;
  case 0x9000:
    // 9XY0: Skip the following instruction if the value of register VX is not
    //       equal to the value of register VY
    if (CHIP.V[X] != CHIP.V[Y]) {
      CHIP.PC += 2;
    }
    DEBUG_PRINT(("Executed 9XY0\n"));
    break;
  case 0xA000:
    // ANNN: Store memory address NNN in register I
    CHIP.I = NNN;
    DEBUG_PRINT(("Executed ANNN\n"));
    break;
  case 0xB000:
    // BNNN: Jump to address NNN + V0
    if (BNNN_LEGACY_BEHAVIOR) {
      CHIP.PC = NNN + CHIP.V[0];
    } else {
      CHIP.PC = NNN + CHIP.V[X];
    }
    DEBUG_PRINT(("Executed B000\n"));
    break;
  case 0xC000:
    // CXNN: Set VX to a random number with a mask of NN
    // No ready RNG implementation; just get end of system ticks
    CHIP.V[X] = timer_get_ticks() & NN;
    DEBUG_PRINT(("Executed C000\n"));
    break;
  case 0xD000:
    // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data
    //       starting at the address stored in I
    //       Set VF to 01 if any set pixels are changed to unset, and 00
    //       otherwise
    {
      int x = CHIP.V[X] & (DISPLAY_WIDTH - 1);
      int y = CHIP.V[Y] & (DISPLAY_HEIGHT - 1);
      int height = CHIP.OPCODE & 0x000F;
      CHIP.V[0xf] = 0;
      for (int row = 0; row < height; row++) {
        unsigned char sprite = CHIP.MEM[CHIP.I + row];
        for (int col = 0; col < 8; col++) {
          if (sprite & (0x80 >> col)) {
            bool pixel = CHIP.PIXELS[y + row][x + col];
            if (pixel) {
              CHIP.V[0xF] = 1;
              draw_pixel(x + col, y + row, false);
            } else {
              draw_pixel(x + col, y + row, true);
            }
            CHIP.PIXELS[y + row][x + col] = !pixel;
          }
          if (x + col == DISPLAY_WIDTH) {
            break;
          }
        }
        if (y + row == DISPLAY_HEIGHT) {
          break;
        }
      }
      DEBUG_PRINT(("Executed DXYN\n"));
    }
    break;
  case 0xE000:
    switch (CHIP.OPCODE & 0xFF) {
    case 0x9E:
      // EX9E: Skip the following instruction if the key corresponding to the
      //       hex value currently stored in register VX is pressed
      if (CHIP.KEYPAD[CHIP.V[X]]) {
        CHIP.PC += 2;
      }
      DEBUG_PRINT(("Executed EX9E\n"));
      break;
    case 0xA1:
      // EXA1: Skip the following instruction if the key corresponding to the
      //       hex value currently stored in register VX is not pressed
      if (!CHIP.KEYPAD[CHIP.V[X]]) {
        CHIP.PC += 2;
      }
      DEBUG_PRINT(("Executed EXA1\n"));
      break;
    }
    break;
  case 0xF000:
    switch (CHIP.OPCODE & 0xFF) {
    case 0x07:
      // FX07: Store the current value of the delay timer in register VX
      CHIP.V[X] = CHIP.DELAY_TIMER;
      DEBUG_PRINT(("Executed FX07\n"));
      break;
    case 0x0A:
      // FX0A: Wait for a keypress and store the result in register VX
      CHIP.PC -= 2;
      for (int i = 0; i < 16; i++) {
        if (CHIP.KEYPAD[i]) {
          CHIP.V[X] = i;
          CHIP.PC += 2;
          break;
        }
      }
      DEBUG_PRINT(("Executed FX0A\n"));
      break;
    case 0x15:
      // FX15: Set the delay timer to the value of register VX
      CHIP.DELAY_TIMER = CHIP.V[X];
      DEBUG_PRINT(("Executed FX15\n"));
      break;
    case 0x18:
      // FX18: Set the sound timer to the value of register VX
      CHIP.SOUND_TIMER = CHIP.V[X];
      DEBUG_PRINT(("Executed FX18\n"));
      break;
    case 0x1E:
      // FX1E: Add the value stored in register VX to register I
      CHIP.V[0xF] = (CHIP.V[X] > 0x0FFF - CHIP.I) ? 1 : 0;
      CHIP.I += CHIP.V[X];
      DEBUG_PRINT(("Executed FX1E\n"));
      break;
    case 0x29:
      // FX29: Set I to the memory address of the sprite data corresponding to
      //       the hexadecimal digit stored in register VX
      CHIP.I = FONT_START + CHIP.V[X] * 5;
      DEBUG_PRINT(("Executed FX29\n"));
      break;
    case 0x33:
      // FX33: Store the binary-coded decimal equivalent of the value stored in
      //       register VX at addresses I, I + 1, and I + 2
      CHIP.MEM[CHIP.I] = CHIP.V[X] / 100;
      CHIP.MEM[CHIP.I + 1] = (CHIP.V[X] / 10) % 10;
      CHIP.MEM[CHIP.I + 2] = (CHIP.V[X] % 100) % 10;
      DEBUG_PRINT(("Executed FX33\n"));
      break;
    case 0x55:
      // FX55: Store the values of registers V0 to VX inclusive in memory
      //       starting at address I
      //       I is set to I + X + 1 after operation
      for (int i = 0; i <= X; i++) {
        CHIP.MEM[CHIP.I + i] = CHIP.V[i];
      }
      if (STR_LDR_LEGACY_BEHAVIOR) {
        CHIP.I += CHIP.V[X] + 1;
      }
      DEBUG_PRINT(("Executed FX55\n"));
      break;
    case 0x65:
      // FX65: Fill registers V0 to VX inclusive with the values stored in
      //       memory starting at address I
      //       I is set to I + X + 1 after operation
      for (int i = 0; i <= X; i++) {
        CHIP.V[i] = CHIP.MEM[CHIP.I + i];
      }
      if (STR_LDR_LEGACY_BEHAVIOR) {
        CHIP.I += CHIP.V[X] + 1;
      }
      DEBUG_PRINT(("Executed FX65\n"));
      break;
    }
    break;
  }
}
#undef X
#undef Y
#undef NN
#undef NNN

#define PROGRAM KEYPAD_TEST
int main() {
  init_keyboard();
  init_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  init_chip();
  load_program(PROGRAM, sizeof(PROGRAM));
  unsigned int last_decrement = 0;
  while (true) {
    emulate_cycle();
    set_keys(CHIP.KEYPAD);
    unsigned int current_tick = timer_get_ticks();
    if (current_tick - last_decrement >= 16666) {
      if (CHIP.DELAY_TIMER > 0) {
        CHIP.DELAY_TIMER--;
      }
      if (CHIP.SOUND_TIMER > 0) {
        CHIP.SOUND_TIMER--;
      }
      last_decrement = current_tick;
    }
    // timer_delay_ms(12);
  }
  return 0;
}
