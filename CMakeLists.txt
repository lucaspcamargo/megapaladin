cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/bluepad32)
set(BTSTACK_ROOT ${BLUEPAD32_ROOT}/external/btstack) # replace built-in in pico-sdk
set(PICO_BTSTACK_PATH ${BTSTACK_ROOT})

include(thirdparty/bluepad32/examples/pico_w/pico_sdk_import.cmake)

project(megapaladin VERSION 2)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(megapaladin
    src/core0.c
    src/core1.c
    src/support.c
    src/button.c
    src/bt_platform.c
    src/ports.c
)


target_include_directories(megapaladin PRIVATE
    src
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include)

# Needed for btstack_config.h / sdkconfig.h
# so that libblupad32 can include them
include_directories(megapaladin src)

# Needed when using BTstack from our branch
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)

target_link_libraries(megapaladin 
    pico_stdlib
    pico_multicore
    hardware_adc        # internal temp reading
    hardware_pwm        # for power led
    hardware_watchdog   # for reboot

    # bt and bluepad32 stuff
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    bluepad32
)

add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

# use usb serial, not uart
pico_enable_stdio_usb(megapaladin 1)
pico_enable_stdio_uart(megapaladin 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(megapaladin)