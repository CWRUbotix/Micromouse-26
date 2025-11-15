#ifndef MICROMOUSE_PINS_2023_H
#define MICROMOUSE_PINS_2023_H

// LEDS
#define LED0 8 // R1
#define LED1 9 // R2
#define LED2 10 // R3
#define LED3 12 // R4
#define RED_LED LED0
#define YELLOW_LED LED1
#define GREEN_LED LED2
#define BLUE_LED LED3
#define DEBUG_LED 13 // Teensy 4.0 Debug LED
#define START_BUTTON 11

// I2C BUSES
#define I2C_LIDAR Wire1

#define BAT_VOLTAGE 23
// Board Section Labels
#define P7 22
#define P6 21
#define P5 20
#define P4 19 // Not updated
#define P3 18

// Map the pins to where the sensor is on the robot
#define LIDAR_FrontShort P7 // LIDAR_CS1
#define LIDAR_BackRight P5
#define LIDAR_FrontRight P6
#define LIDAR_FrontLeft P3 // LIDAR_CS2
#define LIDAR_BackLeft P4

// Encoders
#define ENCODER_RIGHT_1 4
#define ENCODER_RIGHT_2 5
#define ENCODER_LEFT_1 6
#define ENCODER_LEFT_2 7

// Motors
#define MOTORRIGHT_1 0
#define MOTORRIGHT_2 1
#define MOTORLEFT_1 2
#define MOTORLEFT_2 3

#endif

