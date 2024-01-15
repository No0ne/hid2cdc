cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(hid2cdc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(hid2cdc hid2cdc.c)

add_compile_definitions(CTRLALTDEL=3) # ctrl-alt-del reset output gpio
#add_compile_definitions(QWERTZ) # qwertz keyboard

pico_set_program_name(hid2cdc "hid2cdc")
pico_set_program_version(hid2cdc "0.9")

pico_enable_stdio_uart(hid2cdc 1)
pico_enable_stdio_usb(hid2cdc 0)

target_include_directories(hid2cdc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(hid2cdc pico_stdlib tinyusb_host tinyusb_board)

pico_add_extra_outputs(hid2cdc)
