//
// Created by nray on 4/14/23.
//

#ifndef GIUSEP_PI_DEFINITIONS_H
#define GIUSEP_PI_DEFINITIONS_H

#include <stdint.h>

// MakerPi pin definitions

#define SPI_SCK 2
#define SPI_CS  1
#define SPI_SO  0
#define HEALTH_LED 28
#define BOILER_RELAY_OUTPUT 3
#define BOILER_SECONDARY_RELAY_OUTPUT 17
// #define WS2818_OUTPUT 4
#define WS2818_OUTPUT 18

// Constants

#define MAX_DUTY_CYCLE 30
#define TARGET_BOILER_TEMPERATURE 90.0
#define Kp 0.8
#define Ki 0.05
#define Kd 0.005
#define WS2812_BITS_PER_COLOR 8
#define WS2812_BITS_PER_PIXEL (WS2812_BITS_PER_COLOR * 3)
#define WS2812_PIXEL_COUNT 2

// Structs

typedef struct ws2812_pixel_t {
    uint8_t bit_padding;
    uint8_t blue;
    uint8_t red;
    uint8_t green;
} ws2812_pixel_t, *p_ws2812_pixel_t;

typedef struct ws2812_array_t {
    ws2812_pixel_t pixels[WS2812_PIXEL_COUNT];
} ws2812_array_t, *p_ws2812_array_t;

typedef struct boiler_control_t {
    float current_boiler_temperature;
    float pid_error;
    float pid_previous_error;
    float proportional_gain;
    float integral_gain;
    float derivative_gain;
    double duty_cycle;
    uint8_t overheat_count;

} boiler_control_t, *p_boiler_control_t;

// Enums

enum LED_MODE {
    NORMAL,
    SPI_READ_ERROR,
    OVERHEAT_ERROR,

};


// Prototypes
void refresh_leds(p_ws2812_array_t ptr_pixel_array);

void update_all_pixels(p_ws2812_array_t ptr_pixel_array, uint8_t red, uint8_t green, uint8_t blue);

void init_leds(p_ws2812_array_t ptr_pixel_array);

#endif //GIUSEP_PI_DEFINITIONS_H
