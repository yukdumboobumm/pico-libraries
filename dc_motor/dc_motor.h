#ifndef DC_MOTOR_H
#define DC_MOTOR_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "dc_motor.pio.h"

//DEFINES
#ifdef DEBUG
    #ifndef DEBUG_PRINT
        #define DEBUG_PRINT(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
    #endif
#else
    #define DEBUG_PRINT(fmt, ...) (void)0
#endif

//forward declaration of struct
typedef struct DC_MOTOR DC_MOTOR;

//Function Declarations
void initMotor(DC_MOTOR *, uint, uint, uint, uint);//&MOTOR, R-ENABLE, L-ENABLE, R-PWM, L-PWM
void initMotorPWM(DC_MOTOR *, uint);//&MOTOR, FREQUENCY, DUTY CYCLE, PIO NUM
void initMotorPIO(DC_MOTOR *, PIO);//&MOTOR
void stopMotor(DC_MOTOR *);//MOTOR
void runMotor(DC_MOTOR *, bool);//&MOTOR, DIRECTION
void setMotorSpeed(DC_MOTOR *, float);//&MOTOR, SPEED

struct DC_MOTOR
{
    //motor variables
    uint16_t freq; //in hz
    float dutyCycle; //percentage 0-100.0
    bool dir; //current rotation direction
    bool runFlag; //are we running
    bool speedChange;
    
    //pins
    uint R_ENABLE_PIN;
    uint L_ENABLE_PIN;
    uint R_PWM_PIN;
    uint L_PWM_PIN;
    uint LOWEST_PWM; //PIO state machine will take the lowest # pin and initialize up.
    
    //PIO number and state machine 
    PIO PIO_NUM;
    uint SM_NUM;
    uint OFFSET;

    //SM values 
    float clockDiv;
    uint32_t period;
    uint32_t pwmOnCycles;
    uint32_t pwmOffCycles;
};

#endif // DC_MOTOR_H
