#ifndef HACHIP_H
#define HACHIP_H
#include <stdbool.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#define PHYSICAL_WIDTH 64
#define PHYSICAL_HEIGHT 32

// https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#stack
// https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
typedef struct {
  unsigned short OPCODE;
  // 4 kilobytes of RAM
  // 0x000-0x1FF - interpreter & font
  // 0x200-0xFFF - program & RAM
  unsigned char MEM[4096];
  // 16 8-bit (one byte) general-purpose variable registers
  // (V0 to VF)
  unsigned char V[16];
  // 16-bit index register I (point to mem location)
  unsigned short I;
  // 16-bit program counter
  unsigned short PC;
  // 16 level stack
  unsigned short STACK[16];
  unsigned short SP;
  // pixel states (64 * 32)
  bool (*PIXELS)[64];
  // hex keypad
  unsigned char KEYPAD[16];
  // two timer registers that decrement at 60 Hz
  unsigned char DELAY_TIMER;
  unsigned char SOUND_TIMER;
  // set when graphics need to be redrawn
  bool DRAW_FLAG;
} chip_t;

chip_t CHIP;

void init_keyboard(void);

void init_display(unsigned int width, unsigned int height);

void init_chip(void);

void load_program(unsigned short *);

void emulate_cycle(void);

void draw_graphics(void);

void set_keys(void);

unsigned char font[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

#endif
