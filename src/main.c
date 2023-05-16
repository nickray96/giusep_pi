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
    float celsius = (*input_data >> 3) * 0.25;  // First 3 are control bits

    return celsius;
}

void blink() {
    static int current_led_state = 0;

    current_led_state = !current_led_state;
    gpio_put(HEALTH_LED, current_led_state);

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

double calculate_temperature_error(float current_boiler_temperature) {

    static float error = 0;
    static float previous_error = 0;
    static double proportional_gain = 0;
    static double integral_gain = 0.0;
    static double derivative_gain = 0.0;

    previous_error = error;
    error = TARGET_BOILER_TEMPERATURE - current_boiler_temperature;

    printf("Current Error: %.4f\n", error);
    printf("Current Boiler: %.4f\n", current_boiler_temperature);

    // Time delta is assumed to be 1 second due to boiler_set_duty_cycle
    // only returning after 1 second has elapsed
    proportional_gain = error * Kp;
    if (integral_gain < MAX_DUTY_CYCLE) {
        integral_gain += (error * Ki);
    }
    derivative_gain = ((error - previous_error) * Kd);

    printf("proportional_gain: %.4f\n", proportional_gain);
    printf("integral_gain: %.4f\n", integral_gain);
    printf("derivative_gain: %.4f\n", derivative_gain);

    double output = (proportional_gain + integral_gain + derivative_gain);

    // Clamp the output because we cannot drive higher than MAX_DUTY_CYCLE, or lower than 0
    if (output > MAX_DUTY_CYCLE) {
        output = MAX_DUTY_CYCLE;
    }
    if (output < 0) {
        output = 0.0;
    }

    return output;

}

void boiler_set_duty_cycle(double duty_cycle) {
    // Set the boiler on for (duty_cycle/MAX_DUTY_CYCLE)*1000 milliseconds
    // Does not PWM the boiler because SSR relay will pulse electrical system in house
    uint16_t ms_to_boil = (uint16_t) ((duty_cycle / MAX_DUTY_CYCLE) * 1000);
    uint16_t ms_to_idle = 1000 - ms_to_boil;
    printf("Enabling boiler for %d ms\n", ms_to_boil);
    printf("Disabling boiler for %d ms\n", ms_to_idle);
    printf("Duty Cycle: %.4f\n", duty_cycle);

    if (duty_cycle <= 0) {
        set_boiler_relay(false);
        sleep_ms(1000);
        return;
    }
    if (duty_cycle >= MAX_DUTY_CYCLE) {
        set_boiler_relay(true);
        sleep_ms(1000);
        return;
    }

    set_boiler_relay(true);
    sleep_ms(ms_to_boil);
    set_boiler_relay(false);
    sleep_ms(ms_to_idle);

}


int main() {

    spi_inst_t *spi = spi0;
    uint16_t thermocouple_data = 0x0;
    uint16_t *pThermocoupleData = &thermocouple_data;

    boiler_control_t *boiler_information = NULL;
    boiler_information->duty_cycle = 0;

    double duty_cycle;
    float current_boiler_temperature;
    init_peripherals(spi);

    while (true) {

        current_boiler_temperature = get_celsius(spi, pThermocoupleData);
        boiler_information->current_boiler_temperature = current_boiler_temperature;
        duty_cycle = calculate_temperature_error(current_boiler_temperature);
        boiler_information->duty_cycle = duty_cycle;
        boiler_set_duty_cycle(duty_cycle);
        blink();

    }
    return 0;
};