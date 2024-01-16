/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 No0ne (https://github.com/No0ne)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

typedef uint8_t u8;

u8 const FS = 0x1c;
u8 const GS = 0x1d;
u8 const RS = 0x1e;
u8 const US = 0x1f;

alarm_id_t repeater;
u8 repeat = 0;

bool blinking = false;
bool ctrl = false;
bool alt = false;
bool shift = false;
bool altgr = false;
bool qwertz = false;

u8 kb_addr = 0;
u8 kb_inst = 0;
u8 kb_leds = 0;

u8 prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
u8 seq[] = { 0, 0, 0, 0, 0 };

u8 lower[28];
u8 upper[28];

void cdc_write(u8 seqsize) {
  printf(", CDC bytes:");
  
  for(u8 i = 0; i < seqsize; i++) {
    printf(" %02x", seq[i]);
  }
  
  if(tuh_cdc_mounted(0)) {  
    tuh_cdc_write(0, seq, seqsize);
    tuh_cdc_write_flush(0);
  } else {
    printf(", CDC not mounted!");
  }
}

void cdc_send_key(u8 key) {
  printf("HID code = %02x", key);
  
  if(key == HID_KEY_DELETE || key == HID_KEY_KEYPAD_DECIMAL) {
    
    if(ctrl && (alt || altgr)) {
      printf(" + ctrl + alt, resetting...\n\n");
      gpio_put(CTRLALTDEL, 0);
      board_led_write(0);
      watchdog_enable(100, false);
      while(1);
      
    } else if(key == HID_KEY_KEYPAD_DECIMAL && kb_leds & KEYBOARD_LED_NUMLOCK) {
      seq[0] = qwertz ? ',' : '.';
      
    } else {
      seq[0] = 0x7f;
    }
    cdc_write(1);
    
  } else if(key == HID_KEY_EUROPE_2) {
    if(altgr) {
      seq[0] = '|';
    } else if(shift) {
      seq[0] = '>';
    } else {
      seq[0] = '<';
    }
    cdc_write(1);
    
  } else if(altgr && (key >= HID_KEY_7 && key <= HID_KEY_0 || key == HID_KEY_MINUS || key == HID_KEY_BRACKET_RIGHT)) {
    
    if(key == HID_KEY_7) seq[0] = '{';
    if(key == HID_KEY_8) seq[0] = '[';
    if(key == HID_KEY_9) seq[0] = ']';
    if(key == HID_KEY_0) seq[0] = '}';
    if(key == HID_KEY_MINUS) seq[0] = '\\';
    if(key == HID_KEY_BRACKET_RIGHT) seq[0] = '~';
    cdc_write(1);
    
  } else if(key >= HID_KEY_F1 && key <= HID_KEY_F12) {
    
    if(!shift) {
      if(key == HID_KEY_F1)  { seq[2] = '1'; seq[3] = '1'; }
      if(key == HID_KEY_F2)  { seq[2] = '1'; seq[3] = '2'; }
      if(key == HID_KEY_F3)  { seq[2] = '1'; seq[3] = '3'; }
      if(key == HID_KEY_F4)  { seq[2] = '1'; seq[3] = '4'; }
      if(key == HID_KEY_F5)  { seq[2] = '1'; seq[3] = '5'; }
      if(key == HID_KEY_F6)  { seq[2] = '1'; seq[3] = '7'; }
      if(key == HID_KEY_F7)  { seq[2] = '1'; seq[3] = '8'; }
      if(key == HID_KEY_F8)  { seq[2] = '1'; seq[3] = '9'; }
      if(key == HID_KEY_F9)  { seq[2] = '2'; seq[3] = '0'; }
      if(key == HID_KEY_F10) { seq[2] = '2'; seq[3] = '1'; }
    } else {
      if(key == HID_KEY_F1)  { seq[2] = '2'; seq[3] = '3'; }
      if(key == HID_KEY_F2)  { seq[2] = '2'; seq[3] = '4'; }
      if(key == HID_KEY_F3)  { seq[2] = '2'; seq[3] = '5'; }
      if(key == HID_KEY_F4)  { seq[2] = '2'; seq[3] = '6'; }
      if(key == HID_KEY_F5)  { seq[2] = '2'; seq[3] = '8'; }
      if(key == HID_KEY_F6)  { seq[2] = '2'; seq[3] = '9'; }
      if(key == HID_KEY_F7)  { seq[2] = '3'; seq[3] = '1'; }
      if(key == HID_KEY_F8)  { seq[2] = '3'; seq[3] = '2'; }
      if(key == HID_KEY_F9)  { seq[2] = '3'; seq[3] = '3'; }
      if(key == HID_KEY_F10) { seq[2] = '3'; seq[3] = '4'; }
    }
    
    if(key == HID_KEY_F11) { seq[2] = '2'; seq[3] = '3'; }
    if(key == HID_KEY_F12) { seq[2] = '2'; seq[3] = '4'; }
    
    seq[0] = '\e';
    seq[1] = '[';
    seq[4] = '~';
    cdc_write(5);
    
  } else if(key >= HID_KEY_INSERT && key <= HID_KEY_ARROW_UP) {
    
    seq[0] = '\e';
    seq[1] = '[';
    seq[3] = '~';
    
    if(key >= HID_KEY_ARROW_RIGHT) {
      if(key == HID_KEY_ARROW_UP)    seq[2] = 'A';
      if(key == HID_KEY_ARROW_DOWN)  seq[2] = 'B';
      if(key == HID_KEY_ARROW_RIGHT) seq[2] = 'C';
      if(key == HID_KEY_ARROW_LEFT)  seq[2] = 'D';
      cdc_write(3);
    } else {
      if(key == HID_KEY_HOME)      seq[2] = '1';
      if(key == HID_KEY_INSERT)    seq[2] = '2';
      if(key == HID_KEY_END)       seq[2] = '4';
      if(key == HID_KEY_PAGE_UP)   seq[2] = '5';
      if(key == HID_KEY_PAGE_DOWN) seq[2] = '6';
      cdc_write(4);
    }
    
  } else if(ctrl && (key == HID_KEY_SPACE ||
    key >= HID_KEY_BRACKET_LEFT && key <= HID_KEY_BACKSLASH ||
    key >= HID_KEY_GRAVE        && key <= HID_KEY_SLASH)) {
    
    if(key == HID_KEY_SPACE)         seq[0] = 0;
    if(key == HID_KEY_BRACKET_LEFT)  seq[0] = '\e';
    if(key == HID_KEY_BACKSLASH)     seq[0] = FS;
    if(key == HID_KEY_COMMA)         seq[0] = FS;
    if(key == HID_KEY_BRACKET_RIGHT) seq[0] = GS;
    if(key == HID_KEY_GRAVE)         seq[0] = RS;
    if(key == HID_KEY_PERIOD)        seq[0] = RS;
    if(key == HID_KEY_SLASH)         seq[0] = US;
    cdc_write(1);
    
  } else if(key >= HID_KEY_A && key <= HID_KEY_Z) {
    
    if(qwertz && key == HID_KEY_Y) {
      key = HID_KEY_Z;
    } else if(qwertz && key == HID_KEY_Z) {
      key = HID_KEY_Y;
    }
    
    key -= HID_KEY_A;
    
    if(ctrl) {
      seq[0] = key + 1;
      
    } else if(qwertz && altgr && key == HID_KEY_Q - HID_KEY_A) {
      seq[0] = '@';
      
    } else if(shift || kb_leds & KEYBOARD_LED_CAPSLOCK) {
      seq[0] = key + 'A';
      
    } else {
      seq[0] = key + 'a';
    }
    
    cdc_write(1);
    
  } else if(key >= HID_KEY_1 && key <= HID_KEY_SLASH) {
    
    if(shift) {
      seq[0] = upper[key - HID_KEY_1];
    } else {
      seq[0] = lower[key - HID_KEY_1];
    }
    if(seq[0] != '\a') cdc_write(1);
    
  } else if(key >= HID_KEY_KEYPAD_DIVIDE && key <= HID_KEY_KEYPAD_0) {
    
    if(key >= HID_KEY_KEYPAD_1 && !(kb_leds & KEYBOARD_LED_NUMLOCK)) {
      seq[0] = '\e';
      seq[1] = '[';
      seq[3] = '~';
      
      if(key == HID_KEY_KEYPAD_0 || key % 2) {
        if(key == HID_KEY_KEYPAD_7) seq[2] = '1';
        if(key == HID_KEY_KEYPAD_0) seq[2] = '2';
        if(key == HID_KEY_KEYPAD_1) seq[2] = '4';
        if(key == HID_KEY_KEYPAD_9) seq[2] = '5';
        if(key == HID_KEY_KEYPAD_3) seq[2] = '6';
        if(key != HID_KEY_KEYPAD_5) cdc_write(4);
      } else {
        if(key == HID_KEY_KEYPAD_8) seq[2] = 'A';
        if(key == HID_KEY_KEYPAD_2) seq[2] = 'B';
        if(key == HID_KEY_KEYPAD_6) seq[2] = 'C';
        if(key == HID_KEY_KEYPAD_4) seq[2] = 'D';
        cdc_write(3);
      }
    } else {
      seq[0] = "/*-+\r1234567890"[key - HID_KEY_KEYPAD_DIVIDE];
      cdc_write(1);
    }
    
  }
  
  printf("\n");
}

void kb_set_leds() {
  if(kb_addr) {
    printf("HID device address = %d, instance = %d, LEDs = %d\n", kb_addr, kb_inst, kb_leds);
    tuh_hid_set_report(kb_addr, kb_inst, 0, HID_REPORT_TYPE_OUTPUT, &kb_leds, sizeof(kb_leds));
  }
}

void kb_set_led(u8 led) {
  kb_leds ^= led;
  kb_set_leds();
}

int64_t blink_callback(alarm_id_t id, void *user_data) {
  if(blinking) {
    kb_leds = KEYBOARD_LED_NUMLOCK | KEYBOARD_LED_CAPSLOCK | KEYBOARD_LED_SCROLLLOCK;
    kb_set_leds();
    blinking = false;
    return 500000;
  }
  
  kb_leds = gpio_get(NUMLOCK) ? 0 : KEYBOARD_LED_NUMLOCK;
  kb_set_leds();
  return 0;
}

void kb_reset() {
  repeat = 0;
  ctrl = false;
  alt = false;
  shift = false;
  altgr = false;
  blinking = true;
  add_alarm_in_ms(50, blink_callback, NULL, false);
}

int64_t repeat_callback(alarm_id_t id, void *user_data) {
  if(repeat) {
    cdc_send_key(repeat);
    return REPEATUS;
  }
  
  repeater = 0;
  return 0;
}

void kb_send_key(u8 key, bool state) {
  if(key > HID_KEY_EUROPE_2 &&
     key < HID_KEY_CONTROL_LEFT ||
     key > HID_KEY_GUI_RIGHT) return;
  
  if(state) {
    repeat = key;
    if(repeater) cancel_alarm(repeater);
    
    if(key == HID_KEY_NUM_LOCK) {
      kb_set_led(KEYBOARD_LED_NUMLOCK);
      
    } else if(key == HID_KEY_CAPS_LOCK) {
      kb_set_led(KEYBOARD_LED_CAPSLOCK);
      
    } else if(key == HID_KEY_SCROLL_LOCK) {
      //kb_set_led(KEYBOARD_LED_SCROLLLOCK);
      
    } else if(key <= HID_KEY_EUROPE_2) {
      repeater = add_alarm_in_ms(DELAYMS, repeat_callback, NULL, false);
      cdc_send_key(key);
    }
  } else {
    if(key == repeat) repeat = 0;
  }
  
  if(key == HID_KEY_CONTROL_LEFT || key == HID_KEY_CONTROL_RIGHT) ctrl = state;
  if(key == HID_KEY_SHIFT_LEFT   || key == HID_KEY_SHIFT_RIGHT  ) shift = state;
  
  if(qwertz) {
    if(key == HID_KEY_ALT_LEFT) alt = state;
    if(key == HID_KEY_ALT_RIGHT) altgr = state;
  } else {
    if(key == HID_KEY_ALT_LEFT   || key == HID_KEY_ALT_RIGHT    ) alt = state;
  }
}

void tuh_cdc_mount_cb(u8 idx) {
  tuh_cdc_itf_info_t itf_info = { 0 };
  tuh_cdc_itf_get_info(idx, &itf_info);
  printf("CDC Interface is mounted: address = %u, itf_num = %u\n", itf_info.daddr, itf_info.bInterfaceNumber);
  kb_reset();
}

void tuh_cdc_umount_cb(u8 idx) {
  tuh_cdc_itf_info_t itf_info = { 0 };
  tuh_cdc_itf_get_info(idx, &itf_info);
  printf("CDC Interface is unmounted: address = %u, itf_num = %u\n", itf_info.daddr, itf_info.bInterfaceNumber);
  kb_reset();
}

void tuh_hid_mount_cb(u8 dev_addr, u8 instance, u8 const* desc_report, uint16_t desc_len) {
  printf("HID device address = %d, instance = %d is mounted", dev_addr, instance);
  
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
    printf(" - keyboard");
    
    if(!kb_addr) {
      printf(", primary");
      kb_addr = dev_addr;
      kb_inst = instance;
    }
    
    tuh_hid_receive_report(dev_addr, instance);
  }
  
  printf("\n"); 
}

void tuh_hid_umount_cb(u8 dev_addr, u8 instance) {
  printf("HID device address = %d, instance = %d is unmounted", dev_addr, instance);
  
  if(dev_addr == kb_addr && instance == kb_inst) {
    printf(" - keyboard, primary");
    kb_addr = 0;
    kb_inst = 0;
  }
  
  printf("\n");
}

void tuh_hid_report_received_cb(u8 dev_addr, u8 instance, u8 const* report, uint16_t len) {
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD && report[1] == 0) {
    
    if(report[0] != prev_rpt[0]) {
      u8 rbits = report[0];
      u8 pbits = prev_rpt[0];
      
      for(u8 j = 0; j < 8; j++) {
        if((rbits & 1) != (pbits & 1)) {
          kb_send_key(HID_KEY_CONTROL_LEFT + j, rbits & 1);
        }
        
        rbits = rbits >> 1;
        pbits = pbits >> 1;
      }
    }
    
    for(u8 i = 2; i < 8; i++) {
      if(prev_rpt[i]) {
        bool brk = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(prev_rpt[i] == report[j]) {
            brk = false;
            break;
          }
        }
        
        if(brk) {
          kb_send_key(prev_rpt[i], false);
        }
      }
      
      if(report[i]) {
        bool make = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(report[i] == prev_rpt[j]) {
            make = false;
            break;
          }
        }
        
        if(make) {
          kb_send_key(report[i], true);
        }
      }
    }
    
    memcpy(prev_rpt, report, sizeof(prev_rpt));
    
  }
  
  tuh_hid_receive_report(dev_addr, instance);
}

void main() {
  board_init();
  tuh_init(BOARD_TUH_RHPORT);
  gpio_init(CTRLALTDEL);
  gpio_init(NUMLOCK);
  gpio_init(QWERTZ);
  gpio_set_dir(CTRLALTDEL, GPIO_OUT);
  gpio_set_dir(NUMLOCK, GPIO_IN);
  gpio_set_dir(QWERTZ, GPIO_IN);
  gpio_pull_up(NUMLOCK);
  gpio_pull_up(QWERTZ);
  
  board_led_write(1);
  gpio_put(CTRLALTDEL, 1);
  qwertz = !gpio_get(QWERTZ);
  strcpy(lower, qwertz ? "1234567890\r\e\b\t \a\a\a+\a#\a\a^,.-"  : "1234567890\r\e\b\t -=[]\\\a;'`,./");
  strcpy(upper, qwertz ? "!\"\a$%&/()=\r\e\b\t ?\a\a*\a'\a\a`;:_" : "!@#$%^&*()\r\e\b\t _+{}|\a:\"~<>?");
  
  printf("\n%s-%s numlock=%u qwertz=%u\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING, !gpio_get(NUMLOCK), qwertz);
  
  while(1) {
    tuh_task();
  }
}
