//
// Created by nray on 4/14/23.
//

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/definitions.h"
#include "hardware/spi.h"
#include <stdio.h>


float get_celsius(spi_inst_t *spi, uint16_t *input_data) {

    gpio_put(SPI_CS, 0);

    spi_read16_blocking(spi, 0, input_data, 1);

    gpio_put(SPI_CS, 1);

    float celcius = (*input_data >> 3) * 0.25;  // First 3 are control bits

    return celcius;
}

void blink() {
    static int current_led_state = 0;

    current_led_state = !current_led_state;
    gpio_put(HEALTH_LED, current_led_state);

}

void set_boiler_relay(bool enable){

    if(!enable){
        gpio_put(BOILER_RELAY_OUTPUT, 0);
    }
    else{
        gpio_put(BOILER_RELAY_OUTPUT, 1);
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

    //Init blinky LED
    gpio_init(HEALTH_LED);
    gpio_set_dir(HEALTH_LED, GPIO_OUT);
    gpio_put(HEALTH_LED, 0);

    // Init boiler switch input
    gpio_init(BOILER_SWITCH_INPUT);
    gpio_set_dir(BOILER_SWITCH_INPUT, GPIO_IN);
    gpio_pull_down(BOILER_SWITCH_INPUT);

    // Init boiler relay output, secondary not used yet but driving low to keep from floating
    gpio_init(BOILER_RELAY_OUTPUT);
    gpio_init(BOILER_SECONDARY_RELAY_OUTPUT);
    gpio_set_dir(BOILER_RELAY_OUTPUT, GPIO_OUT);
    gpio_set_dir(BOILER_SECONDARY_RELAY_OUTPUT, GPIO_OUT);
    gpio_put(BOILER_RELAY_OUTPUT, 1);
    gpio_put(BOILER_SECONDARY_RELAY_OUTPUT, 1);

    set_boiler_relay(false);

}

void calculate_temperature_error(boiler_control_t boiler_information){

    uint64_t time_delta = (boiler_information.ms_time_now - boiler_information.ms_time_last)/1000;

    float error = TARGET_BOILER_TEMPERATURE - boiler_information.current_boiler_temperature;
    boiler_information.current_error = error;
    float proportional_gain = error * Kp;
    float integral_gain = error * time_delta * Ki;

    float output = proportional_gain + integral_gain;

    // Clamp the output because we cannot drive higher than MAX_DUTY_CYCLE, or lower than 0
    if (output > MAX_DUTY_CYCLE){
        output = MAX_DUTY_CYCLE;
    }
    if (output < 0) {
        output = 0;
    }

    boiler_information.duty_cycle = (int)output;  // this is lazy

}

void boiler_set_duty_cycle(boiler_control_t boiler_information){
    // Set the boiler on for (duty_cycle/MAX_DUTY_CYCLE)*1000 milliseconds
    // Does not PWM the boiler because SSR relay will pulse electrical system in house

    if(boiler_information.duty_cycle <= 0){
        set_boiler_relay(false);
        sleep_ms(1000);
    }
    else if(boiler_information.duty_cycle >= MAX_DUTY_CYCLE){
        set_boiler_relay(true);
        sleep_ms(1000);
    }
    else{
        uint16_t ms_to_idle = (boiler_information.duty_cycle/MAX_DUTY_CYCLE)*1000;
        uint16_t ms_to_boil = 1000 - ms_to_idle;
        set_boiler_relay(true);
        sleep_ms(ms_to_boil);
        set_boiler_relay(false);
        sleep_ms(ms_to_idle);
    }
}


int main() {

    spi_inst_t *spi = spi0;
    uint16_t thermocouple_data = 0x0;
    uint16_t *pTCData = &thermocouple_data;

    boiler_control_t boiler_information;
    boiler_information.duty_cycle = 0;
    boiler_information.ms_time_last = 0;

    init_peripherals(spi);

    // TODO: needs to do things from an ISR or something better than sleep_ms
    while (true) {

        absolute_time_t absolute_time = get_absolute_time();
        boiler_information.ms_time_last = to_ms_since_boot(absolute_time);
        boiler_information.current_boiler_temperature = get_celsius(spi, pTCData);

        printf("Celsius: %.4f\n", boiler_information.current_boiler_temperature);
        calculate_temperature_error(boiler_information);

        boiler_set_duty_cycle(boiler_information);
        printf("Current Duty Cycle: %d\n", boiler_information.duty_cycle);
        printf("Current Error: %.4f\n", boiler_information.current_error);
        boiler_information.ms_time_now = to_ms_since_boot(absolute_time);

        blink();

    }
    return 0;
};