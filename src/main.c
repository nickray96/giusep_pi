//
// Created by nray on 4/14/23.
//

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/definitions.h"
#include "include/cli.h"
#include "hardware/spi.h"
#include <stdio.h>

static const uint8_t REG_DEVID = 0x00;

void reg_read(spi_inst_t *spi, const uint8_t reg, uint16_t *dest){

    uint8_t mb = 0;

    // Construct message (set ~W bit high)
    uint8_t msg = 0x80 | (mb << 6) | reg;

    // Read from register
    gpio_put(SPI_CS, 0);
    gpio_put(SPI_SCK, 0);
    sleep_ms(10);
//    spi_write_blocking(spi, &msg, 1);
    spi_read16_blocking(spi, 0, dest, 1);
    gpio_put(SPI_SCK, 1);
    gpio_put(SPI_CS, 1);

}


float get_celsius(const uint16_t *input_data) {

    gpio_put(SPI_CS, 0);
    sleep_ms(10);

    gpio_put(SPI_CS, 1);

    if (*input_data & 0x4) {
        // uh oh, no thermocouple attached!
        return -100;
        // return -100;
    }

    uint16_t temp = *input_data >> 3;

    return temp * 0.25;
}


void init_therm(){
    stdio_init_all();

    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);

    spi_set_format( spi0,      // SPI instance
                    12,   // Number of bits per transfer
                    1,       // Polarity (CPOL)
                    1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SO, GPIO_FUNC_SPI);
}

int main(){

    spi_inst_t *spi = spi0;
    uint16_t data = 0x00;
    uint16_t *pData = &data;

    reg_read(spi, REG_DEVID, pData);
    init_therm();
    spi_init(spi, 1000 * 1000);

    multicore_launch_core1(cli_main);

    while(true) {
        reg_read(spi, REG_DEVID, pData);
        float temp = get_celsius(pData);
        printf("Celsius: %.4f\n", temp);
        sleep_ms(1000);
    }
    return 0;
};