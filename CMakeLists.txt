cmake_minimum_required(VERSION 3.20)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(giusep_pi LANGUAGES C CXX ASM)
set(BOARD pico_sdk)
set(PICO_BOARD pico_w)

add_executable(build_output
        src/main.c
        src/ws2812.c
)

target_include_directories(build_output PUBLIC
        $ENV{PICO_SDK_PATH}
        src/include
)

set_target_properties(build_output PROPERTIES
        CMAKE_C_STANDARD 11
        CMAKE_CXX_STANDARD 17
)

pico_sdk_init()

target_link_libraries(build_output
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_timer
        hardware_pio
        pico_cyw43_arch_none
        pico_btstack_cyw43
        pico_btstack_ble
)
pico_add_extra_outputs(build_output)
pico_enable_stdio_usb(build_output 1)
pico_enable_stdio_uart(build_output 0)