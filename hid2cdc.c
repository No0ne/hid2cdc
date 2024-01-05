#include "bsp/board.h"
#include "tusb.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

u8 const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };
u32 const repeat_us = 50000;
u16 const delay_ms = 250;

alarm_id_t repeater;
u8 repeat = 0;

bool blinking = false;
bool ctrl = false;
bool shift = false;

u8 kb_addr = 0;
u8 kb_inst = 0;
u8 kb_leds = 0;

u8 prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
u8 seq[] = { 0, 0, 0, 0, 0 };

void cdc_write(u8 seqsize) {
  if(tuh_cdc_mounted(0)) {
    printf(", CDC bytes:");
    
    for(u8 i = 0; i < seqsize; i++) {
      printf(" %02x", seq[i]);
    }
    
    tuh_cdc_write(0, seq, seqsize);
    tuh_cdc_write_flush(0);
  }
}

void cdc_send_key(u8 key) {
  printf("HID code = %02x", key);
  
  if(key == 0x4c) {
    
    seq[0] = 0x7f;
    cdc_write(1);
    
  } else if(key >= 0x3a && key <= 0x45) {
    
    if(!shift) {
      if(key == 0x3a) { seq[2] = 0x31; seq[3] = 0x31; } // VK_F1
      if(key == 0x3b) { seq[2] = 0x31; seq[3] = 0x32; } // VK_F2
      if(key == 0x3c) { seq[2] = 0x31; seq[3] = 0x33; } // VK_F3
      if(key == 0x3d) { seq[2] = 0x31; seq[3] = 0x34; } // VK_F4
      if(key == 0x3e) { seq[2] = 0x31; seq[3] = 0x35; } // VK_F5
      if(key == 0x3f) { seq[2] = 0x31; seq[3] = 0x37; } // VK_F6
      if(key == 0x40) { seq[2] = 0x31; seq[3] = 0x38; } // VK_F7
      if(key == 0x41) { seq[2] = 0x31; seq[3] = 0x39; } // VK_F8
      if(key == 0x42) { seq[2] = 0x32; seq[3] = 0x30; } // VK_F9
      if(key == 0x43) { seq[2] = 0x32; seq[3] = 0x31; } // VK_F10
      if(key == 0x44) { seq[2] = 0x32; seq[3] = 0x33; } // VK_F11
      if(key == 0x45) { seq[2] = 0x32; seq[3] = 0x34; } // VK_F12
    } else {
      if(key == 0x3c) { seq[2] = 0x32; seq[3] = 0x35; } // VK_F13
      if(key == 0x3d) { seq[2] = 0x32; seq[3] = 0x36; } // VK_F14
      if(key == 0x3e) { seq[2] = 0x32; seq[3] = 0x38; } // VK_F15
      if(key == 0x3f) { seq[2] = 0x32; seq[3] = 0x39; } // VK_F16
      if(key == 0x40) { seq[2] = 0x33; seq[3] = 0x31; } // VK_F17
      if(key == 0x41) { seq[2] = 0x33; seq[3] = 0x32; } // VK_F18
      if(key == 0x42) { seq[2] = 0x33; seq[3] = 0x33; } // VK_F19
      if(key == 0x43) { seq[2] = 0x33; seq[3] = 0x34; } // VK_F20
    }
    
    seq[0] = 0x1b;
    seq[1] = 0x5b;
    seq[4] = 0x7e;
    cdc_write(5);
    
  } else if(key >= 0x49 && key <= 0x52) {
    seq[0] = 0x1b;
    seq[1] = 0x5b;
    
    if(key >= 0x4f) {
      
      if(key == 0x52) seq[2] = 0x41; // VK_UP
      if(key == 0x51) seq[2] = 0x42; // VK_DOWN
      if(key == 0x4f) seq[2] = 0x43; // VK_RIGHT
      if(key == 0x50) seq[2] = 0x44; // VK_LEFT
      cdc_write(3);
      
    } else {
      
      if(key == 0x4a) seq[2] = 0x31; // VK_HOME
      if(key == 0x49) seq[2] = 0x32; // VK_INSERT
      if(key == 0x4d) seq[2] = 0x34; // VK_END
      if(key == 0x4b) seq[2] = 0x35; // VK_PRIOR
      if(key == 0x4e) seq[2] = 0x36; // VK_NEXT
      seq[3] = 0x7e;
      cdc_write(4);
      
    }
  } else {
    if(ctrl && (key == 0x2c || key == 0x2f || key == 0x30 || key == 0x31 || key == 0x2c || key == 0x2c)) {
      if(key == 0x2c) seq[0] = 0;
      if(key == 0x2f) seq[0] = 0x1b;
      if(key == 0x31) seq[0] = 0x1c;
      if(key == 0x30) seq[0] = 0x1d;
      cdc_write(1);
      
    } else if(ctrl && key >= 0x04 && key <= 0x1d) {
      seq[0] = key - 3;
      cdc_write(1);
      
    } else {
      seq[0] = keycode2ascii[key][shift];
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
  if(led == 2) shift = kb_leds & 0x2;
  kb_set_leds();
}

int64_t blink_callback(alarm_id_t id, void *user_data) {
  if(blinking) {
    kb_leds = 7;
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

void kb_send_key(u8 key, bool state, u8 modifiers) {
  if(key > 0x73 && key < 0xe0 && key > 0xe7) return;
  
  if(state) {
    if(key == 0x53) kb_set_led(1);
    if(key == 0x39) kb_set_led(2);
    if(key == 0x47) kb_set_led(4);
    
    repeat = key;
    if(repeater) cancel_alarm(repeater);
    
    if(key < 0x73 && key != 0x53 && key != 0x39 && key != 0x47) {
      repeater = add_alarm_in_ms(delay_ms, repeat_callback, NULL, false);
      cdc_send_key(key);
    }
  } else {
    if(key == repeat) repeat = 0;
  }
  
  if(key == 0xe0 || key == 0xe4) ctrl = state;
  if((key == 0xe1 || key == 0xe5) && !(kb_leds & 0x2)) shift = state;
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
        if((rbits & 0x1) != (pbits & 0x1)) {
          kb_send_key(j + 0xe0, rbits & 0x1, report[0]);
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
          kb_send_key(prev_rpt[i], false, report[0]);
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
          kb_send_key(report[i], true, report[0]);
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
  
  printf("\n%s-%s\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  
  while(1) {
    tuh_task();
  }
}
