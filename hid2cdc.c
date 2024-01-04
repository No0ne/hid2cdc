#include "bsp/board.h"
#include "tusb.h"

static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };
uint8_t prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t esccode[] = { 0x1b, 0x5b, 0, 0, 0, 0 };

void tuh_cdc_mount_cb(uint8_t idx) {
  tuh_cdc_itf_info_t itf_info = { 0 };
  tuh_cdc_itf_get_info(idx, &itf_info);

  printf("CDC Interface is mounted: address = %u, itf_num = %u\r\n", itf_info.daddr, itf_info.bInterfaceNumber);

  cdc_line_coding_t line_coding = { 0 };
  if(tuh_cdc_get_local_line_coding(idx, &line_coding)) {
    printf("  Baudrate: %lu, Stop Bits : %u\r\n", line_coding.bit_rate, line_coding.stop_bits);
    printf("  Parity  : %u, Data Width: %u\r\n", line_coding.parity, line_coding.data_bits);
  }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  
  if(tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD && report[1] == 0) {
    bool ctrl = report[0] & 0x01 || report[0] & 0x10;
    bool shift = report[0] & 0x02 || report[0] & 0x20;
    
    for(uint8_t i = 2; i < 8; i++) {
      
      if(report[i]) {
        bool make = true;
        
        for(uint8_t j = 2; j < 8; j++) {
          if(report[i] == prev_rpt[j]) {
            make = false;
            break;
          }
        }
        
        if(make && tuh_cdc_mounted(0)) {
          if(report[i] >= 0x3a && report[i] <= 0x45) {
            
            //esccode[2] = 0x5b;
            if(!shift) {
              if(report[i] == 0x3a) { esccode[2] = 0x11; esccode[3] = 0x11; } // VK_F1
              if(report[i] == 0x3b) { esccode[2] = 0x11; esccode[3] = 0x12; } // VK_F2
              if(report[i] == 0x3c) { esccode[2] = 0x11; esccode[3] = 0x13; } // VK_F3
              if(report[i] == 0x3d) { esccode[2] = 0x11; esccode[3] = 0x14; } // VK_F4
              if(report[i] == 0x3e) { esccode[2] = 0x11; esccode[3] = 0x15; } // VK_F5
              if(report[i] == 0x3f) { esccode[2] = 0x11; esccode[3] = 0x17; } // VK_F6
              if(report[i] == 0x40) { esccode[2] = 0x11; esccode[3] = 0x18; } // VK_F7
              if(report[i] == 0x41) { esccode[2] = 0x11; esccode[3] = 0x19; } // VK_F8
              if(report[i] == 0x42) { esccode[2] = 0x12; esccode[3] = 0x10; } // VK_F9
              if(report[i] == 0x43) { esccode[2] = 0x12; esccode[3] = 0x11; } // VK_F10
              if(report[i] == 0x44) { esccode[2] = 0x12; esccode[3] = 0x13; } // VK_F11
              if(report[i] == 0x45) { esccode[2] = 0x12; esccode[3] = 0x14; } // VK_F12
            } else {
              if(report[i] == 0x3c) { esccode[2] = 0x12; esccode[3] = 0x15; } // VK_F13
              if(report[i] == 0x3d) { esccode[2] = 0x12; esccode[3] = 0x16; } // VK_F14
              if(report[i] == 0x3e) { esccode[2] = 0x12; esccode[3] = 0x18; } // VK_F15
              if(report[i] == 0x3f) { esccode[2] = 0x12; esccode[3] = 0x19; } // VK_F16
              if(report[i] == 0x40) { esccode[2] = 0x13; esccode[3] = 0x11; } // VK_F17
              if(report[i] == 0x41) { esccode[2] = 0x13; esccode[3] = 0x12; } // VK_F18
              if(report[i] == 0x42) { esccode[2] = 0x13; esccode[3] = 0x13; } // VK_F19
              if(report[i] == 0x43) { esccode[2] = 0x13; esccode[3] = 0x14; } // VK_F20
            }
            esccode[4] = 0x7e;
            tuh_cdc_write(0, esccode, 5);
            
          } else if(report[i] >= 0x49 && report[i] <= 0x52 && report[i] != 0x4c) {
            
            if(report[i] >= 0x4f) {
              
              if(report[i] == 0x52) esccode[2] = 0x41; // VK_UP
              if(report[i] == 0x51) esccode[2] = 0x42; // VK_DOWN
              if(report[i] == 0x4f) esccode[2] = 0x43; // VK_RIGHT
              if(report[i] == 0x50) esccode[2] = 0x44; // VK_LEFT
              tuh_cdc_write(0, esccode, 3);
              
            } else {
              
              if(report[i] == 0x4a) esccode[2] = 0x11; // VK_HOME
              if(report[i] == 0x49) esccode[2] = 0x12; // VK_INSERT
              if(report[i] == 0x4d) esccode[2] = 0x14; // VK_END
              if(report[i] == 0x4b) esccode[2] = 0x15; // VK_PRIOR
              if(report[i] == 0x4e) esccode[2] = 0x16; // VK_NEXT
              esccode[3] = 0x7e;
              tuh_cdc_write(0, esccode, 4);
              
            }
            
          } else {
            
            if(ctrl && report[i] >= 0x04 && report[i] <= 0x1d) {
              esccode[0] = report[i] - 3;
              tuh_cdc_write(0, esccode, 1);
              esccode[0] = 0x1b;
            } else {
              tuh_cdc_write(0, &keycode2ascii[report[i]][shift], 1);
            }
            
          }
          
          tuh_cdc_write_flush(0);
        }
      }
    }
    
    memcpy(prev_rpt, report, sizeof(prev_rpt));
  }
  
  tuh_hid_receive_report(dev_addr, instance);
  
}

int64_t repeat_callback(alarm_id_t id, void *user_data) {
  
  if(tuh_cdc_mounted(0)) {
    esccode[2] = 0x41;
    //esccode[3] = 0x7e;
    //esccode[0] = 0x41;
    tuh_cdc_write(0, esccode, 3);
    tuh_cdc_write_flush(0);
    
    printf("gos ");
  }
  
  return 2000000;
}

void main() {
  board_init();
  printf("\n%s-%s\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  tuh_init(BOARD_TUH_RHPORT);
  
  //add_alarm_in_ms(2000, repeat_callback, NULL, false);
  
  while(1) {
    tuh_task();
  }
}
