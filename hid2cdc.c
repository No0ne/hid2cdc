#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "bsp/board.h"
#include "tusb.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

u8 const NUL = 0x00;
u8 const ESC = 0x1b;
u8 const FS = 0x1c;
u8 const GS = 0x1d;
u8 const RS = 0x1e;
u8 const US = 0x1f;
u8 const DEL = 0x7f;
u8 const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };
u32 const repeat_us = 50000;
u16 const delay_ms = 250;

alarm_id_t repeater;
u8 repeat = 0;

bool blinking = false;
bool ctrl = false;
bool alt = false;
bool shift = false;

u8 kb_addr = 0;
u8 kb_inst = 0;
u8 kb_leds = 0;

u8 prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
u8 seq[] = { 0, 0, 0, 0, 0 };

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
  
  if(key == HID_KEY_DELETE) {
    
    if(ctrl && alt) {
      printf(" + ctrl + alt, resetting...\n\n");
      gpio_put(CTRLALTDEL, 0);
      board_led_write(0);
      watchdog_enable(100, false);
      while(1);
    } else {
      seq[0] = DEL;
      cdc_write(1);
    }
    
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
      if(key == HID_KEY_F11) { seq[2] = '2'; seq[3] = '3'; }
      if(key == HID_KEY_F12) { seq[2] = '2'; seq[3] = '4'; }
    } else {
      if(key == HID_KEY_F3)  { seq[2] = '2'; seq[3] = '5'; }
      if(key == HID_KEY_F4)  { seq[2] = '2'; seq[3] = '6'; }
      if(key == HID_KEY_F5)  { seq[2] = '2'; seq[3] = '8'; }
      if(key == HID_KEY_F6)  { seq[2] = '2'; seq[3] = '9'; }
      if(key == HID_KEY_F7)  { seq[2] = '3'; seq[3] = '1'; }
      if(key == HID_KEY_F8)  { seq[2] = '3'; seq[3] = '2'; }
      if(key == HID_KEY_F9)  { seq[2] = '3'; seq[3] = '3'; }
      if(key == HID_KEY_F10) { seq[2] = '3'; seq[3] = '4'; }
    }
    
    seq[0] = ESC;
    seq[1] = '[';
    seq[4] = '~';
    cdc_write(5);
    
  } else if(key >= HID_KEY_INSERT && key <= HID_KEY_ARROW_UP) {
    seq[0] = ESC;
    seq[1] = '[';
    
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
      seq[3] = '~';
      cdc_write(4);
      
    }
  } else {
    if(ctrl && (key == HID_KEY_SPACE ||
      key >= HID_KEY_BRACKET_LEFT && key <= HID_KEY_BACKSLASH ||
      key >= HID_KEY_GRAVE        && key <= HID_KEY_SLASH)) {
      
      if(key == HID_KEY_SPACE)         seq[0] = NUL;
      if(key == HID_KEY_BRACKET_LEFT)  seq[0] = ESC;
      if(key == HID_KEY_BACKSLASH)     seq[0] = FS;
      if(key == HID_KEY_COMMA)         seq[0] = FS;
      if(key == HID_KEY_BRACKET_RIGHT) seq[0] = GS;
      if(key == HID_KEY_GRAVE)         seq[0] = RS;
      if(key == HID_KEY_PERIOD)        seq[0] = RS;
      if(key == HID_KEY_SLASH)         seq[0] = US;
      cdc_write(1);
      
    } else if(ctrl && key >= HID_KEY_A && key <= HID_KEY_Z) {
      seq[0] = key + 1 - HID_KEY_A;
      cdc_write(1);
      
    } else {
      seq[0] = keycode2ascii[key][shift];
      
      if(kb_leds & KEYBOARD_LED_CAPSLOCK && seq[0] >= 'a' && seq[0] <= 'z') {
        seq[0] &= '_';
      }
      
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
  
  kb_leds = 0;
  kb_set_leds();
  return 0;
}

void kb_reset() {
  repeat = 0;
  blinking = true;
  ctrl = false;
  alt = false;
  shift = false;
  add_alarm_in_ms(50, blink_callback, NULL, false);
}

int64_t repeat_callback(alarm_id_t id, void *user_data) {
  if(repeat) {
    cdc_send_key(repeat);
    return repeat_us;
  }
  
  repeater = 0;
  return 0;
}

void kb_send_key(u8 key, bool state) {
  if(key > HID_KEY_KEYPAD_EQUAL &&
     key < HID_KEY_CONTROL_LEFT &&
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
    
    } else if(key < HID_KEY_KEYPAD_EQUAL) {
      repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
      cdc_send_key(key);
    }
  } else {
    if(key == repeat) repeat = 0;
  }
  
  if(key == HID_KEY_CONTROL_LEFT || key == HID_KEY_CONTROL_RIGHT) ctrl = state;
  if(key == HID_KEY_ALT_LEFT     || key == HID_KEY_ALT_RIGHT    ) alt = state;
  if(key == HID_KEY_SHIFT_LEFT   || key == HID_KEY_SHIFT_RIGHT  ) shift = state;
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

void tuh_hid_mount_cb(u8 dev_addr, u8 instance, u8 const* desc_report, u16 desc_len) {
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

void tuh_hid_report_received_cb(u8 dev_addr, u8 instance, u8 const* report, u16 len) {
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
  gpio_set_dir(CTRLALTDEL, GPIO_OUT);
  
  gpio_put(CTRLALTDEL, 1);
  board_led_write(1);
  printf("\n%s-%s\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  
  while(1) {
    tuh_task();
  }
}
