#ifndef HACHIP_H
#define HACHIP_H

unsigned short opcode;

struct {
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
  unsigned char (*pixels)[64];
  // hex keypad
  unsigned char keypad[16];
  // two timer registers that decrement at 60 Hz
  unsigned char delay_timer;
  unsigned char sound_timer;
} chip;

#endif
