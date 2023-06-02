//
// Created by nray on 5/22/23.
//


#include "include/definitions.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include <malloc.h>

// Clock speed is 125MHz, 8ns per cycle, 50 nop per 400ns
#define NOP_DELAY_400ns()   { \
    __asm( \
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    );\
}

#define NOP_DELAY_800ns()   { \
    __asm( \
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n" "nop \n"\
    );\
}


critical_section_t crit_sec;


void init_leds(p_ws2812_array_t ptr_pixel_array) {
    critical_section_init(&crit_sec);
    gpio_init(WS2818_OUTPUT);
    gpio_set_dir(WS2818_OUTPUT, true);
    update_all_pixels(ptr_pixel_array, 0xFF, 0x00, 0x00);
    refresh_leds(ptr_pixel_array);
    sleep_ms(500);
    update_all_pixels(ptr_pixel_array, 0x00, 0xFF, 0x00);
    refresh_leds(ptr_pixel_array);
    sleep_ms(500);
    update_all_pixels(ptr_pixel_array, 0x00, 0x00, 0xFF);
    refresh_leds(ptr_pixel_array);
    sleep_ms(500);
}

void update_all_pixels(p_ws2812_array_t ptr_pixel_array, uint8_t red, uint8_t green, uint8_t blue){
    for (int led = 0; led < WS2812_PIXEL_COUNT; led++) {
        ptr_pixel_array->pixels[led].red = red;
        ptr_pixel_array->pixels[led].green = green;
        ptr_pixel_array->pixels[led].blue = blue;
    }
}

void refresh_leds(p_ws2812_array_t ptr_pixel_array) {
    uint32_t * input_buffer;
    ws2812_array_t pixel_array = *ptr_pixel_array;
    uint32_t * input_buffer_temp = (uint32_t *) &pixel_array;
    input_buffer = (uint32_t *) malloc(sizeof(pixel_array) + sizeof(ws2812_pixel_t));
    // This adds 1 extra "pixel" to the array, using the first WS2812 as a level shifter
    for (uint32_t i = 0; i < (sizeof(pixel_array) / sizeof(ws2812_pixel_t)); i++) {
        if (i == 0) {
            input_buffer[i] = 0x00000000;
        }
        input_buffer[i] = input_buffer_temp[i];
    }

    // This function must be allowed to run to completion each time it gets called
    critical_section_enter_blocking(&crit_sec);
    // WS2812s send the buffer downstream once they receive a full pixel worth

    for (uint32_t pixel = 0; pixel < (sizeof(pixel_array) / sizeof(ws2812_pixel_t)); pixel++) {
        uint32_t pixel_buffer = input_buffer[pixel];
        for (int bit = 0; bit < WS2812_BITS_PER_PIXEL; bit++) {  // Push out each bit per pixel
            if (pixel_buffer & 0x80000000) {
                // logical 1 is high ~800ns, low ~400ns
                gpio_put(WS2818_OUTPUT, 1);
                NOP_DELAY_800ns();
                gpio_put(WS2818_OUTPUT, 0);
                NOP_DELAY_400ns();
            } else {
                // logical 0 is high ~400ns, low ~800ns
                gpio_put(WS2818_OUTPUT, 1);
                NOP_DELAY_400ns();
                gpio_put(WS2818_OUTPUT, 0);
                NOP_DELAY_800ns();
            }
            pixel_buffer <<= 1;
        }
    }
    free(input_buffer);
    critical_section_exit(&crit_sec);

}