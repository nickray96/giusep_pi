//
// Created by nray on 4/14/23.
//

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/definitions.h"
#include "include/cli.h"
#include "hardware/spi.h"
#include <stdio.h>


short get_celsius(spi_inst_t *spi, uint16_t *input_data) {

    gpio_put(SPI_CS, 0);

    spi_read16_blocking(spi, 0, input_data, 1);

    gpio_put(SPI_CS, 1);

    short celcius = (*input_data >> 3) * 0.25;  // First 3 are control bits

    return celcius;
}


void init_therm(spi_inst_t *spi){
    stdio_init_all();

    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, GPIO_OUT);
    gpio_put(SPI_CS, 1);

    spi_init(spi, 1000000);

    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SO, GPIO_FUNC_SPI);

    spi_set_format( spi0,      // SPI instance
                    16,   // Number of bits per transfer
                    1,       // Polarity (CPOL)
                    1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

}

int main(){

    spi_inst_t *spi = spi0;
    uint16_t data = 0x0;
    uint16_t *pData = &data;

    init_therm(spi);


    // This launches stdio_init_all, don't do it again
//    multicore_launch_core1(cli_main);  Prob going to gid rid of this and do something more simple
    gpio_init(HEALTH_LED);
    gpio_set_dir(HEALTH_LED, GPIO_OUT);

    while(true) {

        float temp = get_celsius(spi, pData);
        printf("Celsius: %.4f\n", temp);

        gpio_put(HEALTH_LED, 1);
        sleep_ms(1000);
        gpio_put(HEALTH_LED, 0);
        sleep_ms(1000);
    }
    return 0;
};