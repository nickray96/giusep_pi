//
// Created by nray on 4/14/23.
//

#ifndef GIUSEP_PI_DEFINITIONS_H
#define GIUSEP_PI_DEFINITIONS_H

// MakerPi pin definitions
#define SPI_SCK 2
#define SPI_CS  1
#define SPI_SO  0
#define HEALTH_LED 28
#define BOILER_SWITCH_INPUT 4
#define BOILER_RELAY_OUTPUT 16
#define BOILER_SECONDARY_RELAY_OUTPUT 17

// Constants

#define MAX_DUTY_CYCLE 30
#define TARGET_BOILER_TEMPERATURE 90.0
#define Kp 0.8
#define Ki 0.05
#define Kd 0.005

typedef struct {
    float current_boiler_temperature;
    double duty_cycle;
} boiler_control_t;


#endif //GIUSEP_PI_DEFINITIONS_H
