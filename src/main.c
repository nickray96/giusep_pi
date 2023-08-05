//
// Created by nray on 4/14/23.
//

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/definitions.h"
#include "hardware/spi.h"
#include <stdio.h>

boiler_control_t boiler_information = {0};
ws2812_array_t pixel_array = {0};
enum LED_MODE ledMode;

float get_celsius(spi_inst_t *spi, uint16_t *input_data) {

    gpio_put(SPI_CS, 0);

    while(!spi_is_readable(spi)){
        // This will never exit until SPI comes back alive
        ledMode = SPI_READ_ERROR;
    }
    ledMode = NORMAL;
    spi_read16_blocking(spi, 0, input_data, 1);
    gpio_put(SPI_CS, 1);
    float celsius = (*input_data >> 3) * 0.25;  // First 3 are control bits

    return celsius;
}

void blink() {
    gpio_put(HEALTH_LED, !gpio_get(HEALTH_LED));
}

void set_boiler_relay(bool enable) {

    if (!enable) {
        gpio_put(BOILER_RELAY_OUTPUT, 1);
    } else {
        gpio_put(BOILER_RELAY_OUTPUT, 0);
    }
}


void init_peripherals(spi_inst_t *spi) {
    stdio_init_all();

    // Init SPI
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);

    spi_init(spi, 1000000);

    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SO, GPIO_FUNC_SPI);

    spi_set_format(spi0,      // SPI instance
                   16,   // Number of bits per transfer
                   1,       // Polarity (CPOL)
                   1,      // Phase (CPHA)
                   SPI_MSB_FIRST);

    // Init blinky LED
    gpio_init(HEALTH_LED);
    gpio_set_dir(HEALTH_LED, GPIO_OUT);
    gpio_put(HEALTH_LED, 0);

    // Init boiler relay output, secondary not used yet but driving low to keep from floating
    gpio_init(BOILER_RELAY_OUTPUT);
    gpio_init(BOILER_SECONDARY_RELAY_OUTPUT);
    gpio_set_dir(BOILER_RELAY_OUTPUT, GPIO_OUT);
    gpio_set_dir(BOILER_SECONDARY_RELAY_OUTPUT, GPIO_OUT);
    gpio_put(BOILER_RELAY_OUTPUT, 1);
    gpio_put(BOILER_SECONDARY_RELAY_OUTPUT, 1);

    set_boiler_relay(false);

}

double calculate_temperature_error() {

    boiler_information.pid_previous_error = boiler_information.pid_error;
    boiler_information.pid_error = TARGET_BOILER_TEMPERATURE - boiler_information.current_boiler_temperature;

    // If above target temperature for too long, indicate to user
    // If overheating all the time, the PID gains are probably wrong and should be adjusted
    if (boiler_information.current_boiler_temperature > TARGET_BOILER_TEMPERATURE){
        if (boiler_information.overheat_count > 10) {
            ledMode = OVERHEAT_ERROR;
        }
        else{
            boiler_information.overheat_count += 1;
        }
    }
    else{
        boiler_information.overheat_count = 0;
        ledMode = NORMAL;
    }

    printf("Current Error: %.4f\n", boiler_information.pid_error);
    printf("Current Boiler: %.4f\n", boiler_information.current_boiler_temperature);

    // Time delta is assumed to be 1 second due to boiler_set_duty_cycle
    // only returning after 1 second has elapsed
    boiler_information.proportional_gain = boiler_information.pid_error * Kp;
    if (boiler_information.integral_gain < MAX_DUTY_CYCLE) {
        boiler_information.integral_gain += (boiler_information.pid_error * Ki);
    }
    boiler_information.derivative_gain = ((boiler_information.pid_error - boiler_information.pid_previous_error) * Kd);

    printf("proportional_gain: %.4f\n", boiler_information.proportional_gain);
    printf("integral_gain: %.4f\n", boiler_information.integral_gain);
    printf("derivative_gain: %.4f\n", boiler_information.derivative_gain);

    double output = (boiler_information.proportional_gain + boiler_information.integral_gain + boiler_information.derivative_gain);

    // Clamp the output because we cannot drive higher than MAX_DUTY_CYCLE, or lower than 0
    if (output > MAX_DUTY_CYCLE) {
        output = MAX_DUTY_CYCLE;
    }
    if (output < 0) {
        output = 0.0;
    }

    return output;

}

void boiler_set_duty_cycle() {
    // Set the boiler on for (duty_cycle/MAX_DUTY_CYCLE)*1000 milliseconds
    // Does not PWM the boiler because SSR relay will pulse electrical system in house
    uint16_t ms_to_boil = (uint16_t) ((boiler_information.duty_cycle / MAX_DUTY_CYCLE) * 1000);
    uint16_t ms_to_idle = 1000 - ms_to_boil;
    printf("Enabling boiler for %d ms\n", ms_to_boil);
    printf("Disabling boiler for %d ms\n", ms_to_idle);
    printf("Duty Cycle: %.4f\n", boiler_information.duty_cycle);

    if (boiler_information.duty_cycle <= 0) {
        set_boiler_relay(false);
        sleep_ms(1000);
        return;
    }
    if (boiler_information.duty_cycle >= MAX_DUTY_CYCLE) {
        set_boiler_relay(true);
        sleep_ms(1000);
        return;
    }

    set_boiler_relay(true);
    sleep_ms(ms_to_boil);
    set_boiler_relay(false);
    sleep_ms(ms_to_idle);

}

void update_led_panel() {
    uint8_t red_tint = 0x00;
    uint8_t green_tint = 0x00;
    uint8_t blue_tint = 0x00;

    while (true) {

        switch(ledMode){

            case NORMAL:
                // prevent rollover, should be no need to check negative numbers
                if (boiler_information.current_boiler_temperature > TARGET_BOILER_TEMPERATURE) {
                    red_tint = 0xA0;
                    blue_tint = 0x00;
                } else {
                    // The red LED is a lot stronger than blue, so scale it down some
                    red_tint = (uint8_t) (0xA0 * (boiler_information.current_boiler_temperature / TARGET_BOILER_TEMPERATURE));
                    if (boiler_information.current_boiler_temperature < 25) {
                        blue_tint = 0xFF;
                    } else {
                        blue_tint = (uint8_t) (0xFF *
                                               ((TARGET_BOILER_TEMPERATURE - boiler_information.current_boiler_temperature +
                                                 25) / TARGET_BOILER_TEMPERATURE));
                    }
                }

                update_all_pixels(&pixel_array, red_tint, green_tint, blue_tint);
                refresh_leds(&pixel_array);
                sleep_ms(200);

            case OVERHEAT_ERROR:
                red_tint = !red_tint;
                update_all_pixels(&pixel_array, red_tint, green_tint, blue_tint);
                refresh_leds(&pixel_array);
                sleep_ms(500);

            case SPI_READ_ERROR:
                green_tint = !green_tint;
                update_all_pixels(&pixel_array, red_tint, green_tint, blue_tint);
                refresh_leds(&pixel_array);
                sleep_ms(500);
        }
    }
}


int main() {

    spi_inst_t *spi = spi0;
    uint16_t thermocouple_data = 0x0;
    uint16_t *pThermocoupleData = &thermocouple_data;

    multicore_reset_core1();
    init_peripherals(spi);
    init_leds(&pixel_array);
    ledMode = NORMAL;

    multicore_launch_core1(update_led_panel);

    while (true) {

        boiler_information.current_boiler_temperature = get_celsius(spi, pThermocoupleData);
        boiler_information.duty_cycle = calculate_temperature_error();
        boiler_set_duty_cycle();
        blink();

    }
    return 0;
};