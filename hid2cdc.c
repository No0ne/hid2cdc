#include "bsp/board.h"
#include "tusb.h"

static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };
uint8_t prev_rpt[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

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
          tuh_cdc_write(0, &keycode2ascii[report[i]][report[0] & 0x2 || report[0] & 0x20], 1);
          tuh_cdc_write_flush(0);
        }
      }
    }
    
    memcpy(prev_rpt, report, sizeof(prev_rpt));
  }
  
  tuh_hid_receive_report(dev_addr, instance);
  
}

void main() {
  board_init();
  printf("\n%s-%s\n", PICO_PROGRAM_NAME, PICO_PROGRAM_VERSION_STRING);
  tuh_init(BOARD_TUH_RHPORT);
  
  while (1) {
    tuh_task();
  }
}
