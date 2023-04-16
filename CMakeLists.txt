cmake_minimum_required(VERSION 3.20)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(giusep_pi LANGUAGES C CXX ASM)
set(BOARD pico_sdk)

add_executable(build_output
        src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/src/embedded_cli.c
        src/include/definitions.h src/cli.c src/include/cli.h
        )

set_target_properties(build_output PROPERTIES
        CMAKE_C_STANDARD 11
        CMAKE_CXX_STANDARD 17
)

pico_sdk_init()

target_include_directories(build_output
        PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/include
        )

target_link_libraries(build_output
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_timer
        hardware_pio
)
pico_add_extra_outputs(build_output)
pico_enable_stdio_usb(build_output 1)
pico_enable_stdio_uart(build_output 0)