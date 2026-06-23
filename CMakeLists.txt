cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(auto_whammy_rp2040 C CXX ASM)

pico_sdk_init()

# Use your actual file names (no src/ folder)
add_executable(auto_whammy_rp2040
    srcmain.cpp
)

# Tell the compiler where to find tusb_config.h (in the root)
target_include_directories(auto_whammy_rp2040 PRIVATE .)

target_link_libraries(auto_whammy_rp2040
    pico_stdlib
    hardware_gpio
    hardware_timer
    tinyusb_device
    tinyusb_host
)

target_compile_definitions(auto_whammy_rp2040 PRIVATE
    CFG_TUSB_MCU=OPT_MCU_RP2040
    CFG_TUSB_OS=OPT_OS_PICO
    TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

pico_enable_stdio_uart(auto_whammy_rp2040 0)
pico_enable_stdio_usb(auto_whammy_rp2040 0)

pico_add_extra_outputs(auto_whammy_rp2040)
