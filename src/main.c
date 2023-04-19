//
// Created by nray on 4/14/23.
//

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/definitions.h"
#include "hardware/spi.h"
#include <stdio.h>


short get_celsius(spi_inst_t *spi, uint16_t *input_data) {

    gpio_put(SPI_CS, 0);

    spi_read16_blocking(spi, 0, input_data, 1);

    gpio_put(SPI_CS, 1);

    short celcius = (*input_data >> 3) * 0.25;  // First 3 are control bits

    return celcius;
}

void blink() {
    static int current_led_state = 0;

    current_led_state = !current_led_state;
    gpio_put(HEALTH_LED, current_led_state);

}

void set_boiler_relay(){
    int boiler_switch_state = gpio_get(BOILER_SWITCH_INPUT);

    // Boiler relay is active low
    if(boiler_switch_state != 0){
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

}

int main() {

    spi_inst_t *spi = spi0;
    uint16_t thermocouple_data = 0x0;
    uint16_t *pTCData = &thermocouple_data;

    init_peripherals(spi);

    // TODO: needs to do things from an ISR or something better than sleep_ms
    while (true) {

        float temp = get_celsius(spi, pTCData);
        printf("Celsius: %.4f\n", temp);
        blink();
        set_boiler_relay();
        sleep_ms(1000);
    }
    return 0;
};