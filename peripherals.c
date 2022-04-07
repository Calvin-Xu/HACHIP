#include "peripherals.h"
#include "gl.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupts.h"
#include "interrupts.h"
#include "malloc.h"
#include "printf.h"
#include "ps2_keys.h"
#include "ringbuffer.h"
#include "strings.h"
#include "timer.h"
#include "uart.h"

#define PHYSICAL_WIDTH 800
#define PHYSICAL_HEIGHT 480

#define KEYBOARD_CLOCK GPIO_PIN3
#define KEYBOARD_DATA GPIO_PIN4

int k_display_width;
int k_display_height;
int k_scale;
int k_padding_x;
int k_padding_y;

typedef struct ps2_device ps2_device_t;

ps2_device_t *keyboard;

struct ps2_device {
  unsigned int clock;
  unsigned int data;
  unsigned char scancode;
  rb_t *rb;
};

void on_clock_down(unsigned int pc, void *aux_data) {
  ps2_device_t *dev = (ps2_device_t *)aux_data;
  static unsigned char scancode = 0;
  unsigned int parity = 0;
  static int n = 1;
  if (gpio_check_and_clear_event(dev->clock)) {
    unsigned int bit = gpio_read(dev->data);
    n++;
    // collect scancode
    if (n >= 2 && n <= 9) {
      scancode |= (bit << (n - 2));
      parity += bit;
    }
    // parity check
    if (n == 10) {
      if ((parity + bit) % 2 != 1) {
        n = 1;
        scancode = 0;
      }
    }
    if (n == 11) {
      if (bit != 1) {
        n = 1;
        scancode = 0;
      } else {
        rb_enqueue(dev->rb, scancode);
        scancode = 0;
        n = 1;
      }
    }
  }
}

int i = 0;

unsigned char keyboard_read_scancode(void) {
  printf("%d\n", i++);
  int scancode = 0;
  // while (!rb_dequeue(keyboard->rb, &scancode)) {
  //   continue;
  // }
  if (!rb_dequeue(keyboard->rb, &scancode)) {
    return 0;
  }
  return (unsigned char)scancode;
}

bool ps2_write(ps2_device_t *dev, unsigned char command) {

  // 1)   Bring the Clock line low for at least 100 microseconds.
  gpio_set_output(dev->clock);
  gpio_write(dev->clock, 0);
  timer_delay_us(100);
  // 2)   Bring the Data line low.
  gpio_set_output(dev->data);
  gpio_write(dev->data, 0);
  // 3)   Release the Clock line.
  gpio_set_input(dev->clock);
  // 4)   Wait for the device to bring the Clock line low.
  while (dev->clock == 1) {
    continue;
  }
  // 5)   Set/reset the Data line to send the first data bit
  // 6)   Wait for the device to bring Clock high.
  // 7)   Wait for the device to bring Clock low.
  // 8)   Repeat steps 5-7 for the other seven data bits and the parity bit
  unsigned int parity = 0;
  for (unsigned int i = 0; i < 9; i++) {
    unsigned int bit;
    if (i < 8) {
      bit = (command >> i) & 1;
      parity += bit;
    } else {
      bit = parity % 2 == 0 ? 1 : 0;
    }
    gpio_write(dev->data, bit);
    while (dev->clock == 0) {
      continue;
    }
    while (dev->clock == 1) {
      continue;
    }
  }
  // 9)   Release the Data line.
  gpio_set_input(dev->data);
  // 10) Wait for the device to bring Data low.
  while (dev->data == 1) {
    continue;
  }
  // 11) Wait for the device to bring Clock low.
  while (dev->clock == 1) {
    continue;
  }
  // 12) Wait for the device to release Data and Clock
  while (dev->data == 0) {
    continue;
  }
  return true;
}

void create_ps2_keyboard(unsigned int clock_gpio, unsigned int data_gpio) {
  keyboard = malloc(sizeof(*keyboard));
  keyboard->clock = clock_gpio;
  gpio_set_input(keyboard->clock);
  gpio_set_pullup(keyboard->clock);

  keyboard->data = data_gpio;
  gpio_set_input(keyboard->data);
  gpio_set_pullup(keyboard->data);

  keyboard->rb = rb_new();

  gpio_enable_event_detection(keyboard->clock, GPIO_DETECT_FALLING_EDGE);
  gpio_interrupts_register_handler(keyboard->clock, on_clock_down, keyboard);
}

void init_keyboard(void) {
  gpio_init();
  interrupts_init();
  interrupts_global_enable();
  gpio_interrupts_init();
  gpio_interrupts_enable();
  create_ps2_keyboard(KEYBOARD_CLOCK, KEYBOARD_DATA);
  while (true) {
    unsigned char scancode = keyboard_read_scancode();
    if (scancode == 0xAA) {
      // interrupts_global_disable();
      // gpio_interrupts_disable();
      ps2_write(keyboard, 0x55);
      // interrupts_global_enable();
      // gpio_interrupts_enable();
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

typedef struct {
  enum { KEY_PRESS, KEY_RELEASE } what;
  unsigned char keycode;
} key_action_t;

// note that blocking scancode read is used subsequently
// so that entire sequences are read
key_action_t poll_keyboard_sequence(void) {
  key_action_t action;
  unsigned char keycode = keyboard_read_scancode();
  switch (keycode) {
  case PS2_CODE_EXTENDED:
    keycode = keyboard_read_scancode();
    if (keycode == PS2_CODE_RELEASE) {
      action.what = KEY_RELEASE;
      action.keycode = keyboard_read_scancode();
    } else {
      action.what = KEY_PRESS;
      action.keycode = keycode;
    }
    break;
  case PS2_CODE_RELEASE:
    action.what = KEY_RELEASE;
    action.keycode = keyboard_read_scancode();
    break;
  default:
    action.keycode = keycode;
    action.what = KEY_PRESS;
  }
  return action;
}

void set_keys(bool *keypad) {
  // unsigned char keys[16] = {'x', '1', '2', '3', 'q', 'w', 'e', 'a',
  //                           's', 'd', 'z', 'c', '4', 'r', 'f', 'v'};
  // key_action_t action = poll_keyboard_sequence();
  // if (action.keycode == 0) {
  //   memset(keypad, false, 16);
  //   return;
  // }
  // unsigned char key = ps2_keys[action.keycode].ch;
  // for (int i = 0; i < 16; i++) {
  //   if (key == keys[i]) {
  //     *(keypad + i) = action.what == KEY_PRESS ? true : false;
  //   }
  // }
  unsigned char scancode = keyboard_read_scancode();
  // if (scancode == 0) {
  //   return;
  // }
  printf("%x\n", scancode);
}

void play_sound(bool on) {
  // not implemented
}
