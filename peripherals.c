#include "peripherals.h"
#include "gl.h"
#include "interrupts.h"
#include "keyboard.h"
#include "malloc.h"
#include "ps2.h"
#include "ps2_keys.h"
#include "strings.h"
#include "timer.h"

#define PHYSICAL_WIDTH 800
#define PHYSICAL_HEIGHT 480

int k_display_width;
int k_display_height;
int k_scale;
int k_padding_x;
int k_padding_y;

ps2_device_t *keyboard;

void init_keyboard(void) {
  gpio_init();
  keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
  keyboard = ps2_new(KEYBOARD_CLOCK, KEYBOARD_DATA);
  while (true) {
    if (keyboard_read_scancode() == 0xAA) {
      ps2_write(keyboard, 0x55);
      break;
    }
  }
}

void init_display(int width, int height) {
  k_display_width = width;
  k_display_height = height;
  k_scale = PHYSICAL_WIDTH / k_display_width;
  k_padding_x = (PHYSICAL_WIDTH - k_scale * k_display_width) / 2;
  k_padding_y = (PHYSICAL_HEIGHT - k_scale * k_display_height) / 2;
  gl_init(PHYSICAL_WIDTH, PHYSICAL_HEIGHT, GL_SINGLEBUFFER);
  gl_clear(GL_BLACK);
}

void clear_display(void) { gl_clear(GL_BLACK); }

void draw_pixel(int x, int y, bool is_on) {
  gl_draw_rect(x * k_scale + k_padding_x, y * k_scale + k_padding_y, k_scale,
               k_scale, is_on ? GL_WHITE : GL_BLACK);
}

struct ps2_device {
  unsigned int clock;
  unsigned int data;
};

// incredibly bad hack to get keyboard working
//
// existing keyboard and PS/2 API do not return
// until a key / scancode is read
//
// here is a minimal working version that has an internal
// timeout to return no key pressed

// has a 100us timeout
unsigned int read_bit(ps2_device_t *dev) {
  unsigned int start_time = timer_get_ticks();
  while (gpio_read(dev->clock) == 0) {
    continue;
  }
  while (gpio_read(dev->clock) == 1) {
    if (timer_get_ticks() - start_time > 200) {
      return 2;
    }
    continue;
  }
  return gpio_read(dev->data);
}

unsigned char poll_ps2_scancode(ps2_device_t *dev) {
  unsigned char scan_code = 0;
  while (true) {
    unsigned int start_bit = read_bit(dev);
    while (start_bit != 0) {
      if (start_bit == 2) {
        return 0;
      }
      start_bit = read_bit(dev);
    }
    unsigned int i = 0;
    unsigned int parity = 0;
    while (i < 8) {
      unsigned int bit = read_bit(dev);
      scan_code |= (bit << i++);
      parity += bit;
    }
    if ((parity + read_bit(dev)) % 2 == 1) {
      if (read_bit(dev) == 1) {
        break;
      }
    }
  }
  return scan_code;
}

// note that blocking scancode read is used subsequently
// so that entire sequences are read
key_action_t poll_keyboard_sequence(void) {
  key_action_t action;
  unsigned char keycode = poll_ps2_scancode(keyboard);
  switch (keycode) {
  case PS2_CODE_EXTENDED:
    keycode = ps2_read(keyboard);
    if (keycode == PS2_CODE_RELEASE) {
      action.what = KEY_RELEASE;
      action.keycode = ps2_read(keyboard);
    } else {
      action.what = KEY_PRESS;
      action.keycode = keycode;
    }
    break;
  case PS2_CODE_RELEASE:
    action.what = KEY_RELEASE;
    action.keycode = ps2_read(keyboard);
    break;
  default:
    action.keycode = keycode;
    action.what = KEY_PRESS;
  }
  return action;
}

void set_keys(bool *keypad) {
  unsigned char keys[16] = {'x', '1', '2', '3', 'q', 'w', 'e', 'a',
                            's', 'd', 'z', 'c', '4', 'r', 'f', 'v'};
  key_action_t action = poll_keyboard_sequence();
  if (action.keycode == 0) {
    memset(keypad, false, 16);
    return;
  }
  unsigned char key = ps2_keys[action.keycode].ch;
  for (int i = 0; i < 16; i++) {
    if (key == keys[i]) {
      *(keypad + i) = action.what == KEY_PRESS ? true : false;
    }
  }
}

void play_sound(bool on) {
  // not implemented
}
