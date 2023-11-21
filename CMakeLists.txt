cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(thirdparty/pico-sdk/pico_sdk_init.cmake)

project(megapaladin VERSION 2)

pico_sdk_init()

add_executable(megapaladin
    src/main.c
    src/core1.c
    src/utils.c
    src/button.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(megapaladin pico_stdlib)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(megapaladin)