cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(thirdparty/pico-sdk/pico_sdk_init.cmake)

project(megapaladin VERSION 2)

pico_sdk_init()

add_executable(megapaladin
    src/core0.c
    src/core1.c
    src/support.c
    src/button.c
)

target_link_libraries(megapaladin 
    pico_stdlib
    pico_multicore
    hardware_adc
)

# use usb serial, not uart
pico_enable_stdio_usb(megapaladin 1)
pico_enable_stdio_uart(megapaladin 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(megapaladin)