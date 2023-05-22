// #define DEBUG
#include "dc_motor.h"

//initialize our pins
//&MOTOR, R-ENABLE, L-ENABLE, R-PWM, L-PWM
void initMotor(DC_MOTOR *motor, uint p1, uint p2, uint p3, uint p4)
{
    motor->R_ENABLE_PIN = p1;
    motor->L_ENABLE_PIN = p2;
    motor->R_PWM_PIN = p3;
    motor->L_PWM_PIN = p4;

    motor->LOWEST_PWM = motor->R_PWM_PIN < motor->L_PWM_PIN ? motor->R_PWM_PIN:motor->L_PWM_PIN;

    //initialize gpio for read/write
    gpio_init(motor->R_ENABLE_PIN);
    gpio_init(motor->L_ENABLE_PIN);
    gpio_init(motor->R_PWM_PIN);
    gpio_init(motor->L_PWM_PIN);
    //set gpios to outputs
    gpio_set_dir(motor->R_ENABLE_PIN, GPIO_OUT);
    gpio_set_dir(motor->L_ENABLE_PIN, GPIO_OUT);
    gpio_set_dir(motor->R_PWM_PIN, GPIO_OUT);
    gpio_set_dir(motor->L_PWM_PIN, GPIO_OUT);
    //write initial values to low
    gpio_put(motor->R_ENABLE_PIN, false);
    gpio_put(motor->L_ENABLE_PIN, false);
    gpio_put(motor->R_PWM_PIN, false);
    gpio_put(motor->L_PWM_PIN, false);

    //initialize pwm and state variables to zero
    motor->freq = 0, motor->currentDutyCycle = 0, motor->setDutyCycle, motor->dir = 0, motor->runFlag = 0;
    motor->period = 0, motor->pwmOnCycles = 0, motor->pwmOffCycles = 0;
    DEBUG_PRINT("DC MOTOR PINS\nREn: %d, LEn: %d, RPWM: %d, LPWM: %d\n", motor->R_ENABLE_PIN, motor->L_ENABLE_PIN, motor->R_PWM_PIN, motor->L_PWM_PIN);
}

//initialize the motor with PWM frequency and duty cycle
//&MOTOR, FREQUENCY, DUTY CYCLE, PIO NUM
void initMotorPWM(DC_MOTOR *motor, uint freq) {
    motor->freq = freq;
    //determine pwm period and on/off cycles
    motor->period = (clock_get_hz(clk_sys) / motor->freq) - 7;//dc_motor PIO program has 7 instructions
    DEBUG_PRINT("sys clock: %d\n", clock_get_hz(clk_sys));
    DEBUG_PRINT("PWM frequency: %d\n", motor->freq);
}

//grab a state machine, load, and initialize it
void initMotorPIO(DC_MOTOR *motor, PIO pio) {
    motor->PIO_NUM = pio;
    motor->SM_NUM = pio_claim_unused_sm(motor->PIO_NUM, true);// Get first free state machine in selected PIO
    motor->OFFSET = pio_add_program(motor->PIO_NUM, &dc_motor_program);// Add PIO program to PIO instruction memory, return memory offset
    // pio_sm_claim(motor->PIO_NUM, motor->SM_NUM); //let everyone know we're using this SM
    dc_motor_program_init(motor->PIO_NUM, motor->SM_NUM, motor->OFFSET, motor->LOWEST_PWM);//initialize the SM
    pio_sm_put_blocking(motor->PIO_NUM, motor->SM_NUM, 0);//state machines starts in a 0 duty cycle
    pio_sm_set_enabled(motor->PIO_NUM, motor->SM_NUM, true);//start the SM
    DEBUG_PRINT("Loaded PIO program at %d and started SM: %d\n", motor->OFFSET, motor->SM_NUM);
}

//stop the motor with physical pins and disables the motor's state machine
void stopMotor(DC_MOTOR *motor) {
    DEBUG_PRINT("STOPPING MOTOR\n");
    gpio_put(motor->R_ENABLE_PIN, false);
    gpio_put(motor->L_ENABLE_PIN, false);
    pio_sm_drain_tx_fifo(motor->PIO_NUM, motor->SM_NUM); //safe
    pio_sm_put_blocking(motor->PIO_NUM, motor->SM_NUM, 0);//send our word to the TX FIFO
    motor->runFlag = false;
    motor->currentDutyCycle = 0;
    sleep_ms(2000);
}

//run a motor in the given direction by setting enable pins and starting the staate machine
//&MOTOR, DIRECTION
void runMotor(DC_MOTOR *motor, bool dir, float speed) {
    //check if we actually need to do anything
    if (dir != motor->dir || motor->runFlag == false) {
        if (motor->runFlag && dir != motor->dir) stopMotor(motor);
        uint32_t SM_word = (motor->pwmOffCycles & 0x7fff) << 17; //shift the 15 bit off-cycles to the MSBs of 32 bit word
        SM_word |= (motor->pwmOnCycles & 0x7fff) << 2; //shift 15 bits of on-cycles next, leaving two bits for dir and run
        SM_word |=  dir + 1; //0b10 or 0b01 will set the out-pins in a single instruction
        motor->runFlag = true;
        motor->speedChange = false;
        motor->dir = dir;
        DEBUG_PRINT("RUNNING MOTOR\n");
        pio_sm_drain_tx_fifo(motor->PIO_NUM, motor->SM_NUM); //safe
        pio_sm_put_blocking(motor->PIO_NUM, motor->SM_NUM, SM_word);//send our word to the TX FIFO
        // pio_sm_set_enabled(motor->PIO_NUM, motor->SM_NUM, true);//start the SM
        DEBUG_PRINT("Instruction sent to PIO motor controller\n");
        gpio_put(motor->R_ENABLE_PIN, true);
        gpio_put(motor->L_ENABLE_PIN, true);
    }
}

// //set the motor speed as a percentage of the PWM period (duty cycle)
// void setMotorSpeed(DC_MOTOR *motor, float speed) {
//     motor->dutyCycle = speed;
//     motor->pwmOnCycles = motor->period * motor->dutyCycle / 100U;
//     motor->pwmOffCycles = motor->period - motor->pwmOnCycles;
//     motor->speedChange = true;
//     DEBUG_PRINT("Speed changed to: %.2f\n", motor->dutyCycle);
//     DEBUG_PRINT("with on cycles: %d, off cycles: %d\n", motor->pwmOnCycles, motor->pwmOffCycles);
// }

void runMotorReverse(DC_MOTOR *motor, float speed) {
    bool dir = true;
    if (motor->runFlag && dir != motor->dir) stopMotor(motor);
}

void runMotorForward(DC_MOTOR *motor, float speed) {
    bool dir = true;
    motor->setDutyCycle = speed;
    uint32_t SM_word;
    if (motor->runFlag && dir != motor->dir) stopMotor(motor);
    DEBUG_PRINT("current: %.2f\tset: %.2f\n", motor->currentDutyCycle, motor->setDutyCycle);
    while (motor->currentDutyCycle != motor->setDutyCycle) {
        if ((motor->currentDutyCycle - motor->setDutyCycle < 1) && (motor->currentDutyCycle - motor->setDutyCycle > -1)) {
            motor->currentDutyCycle = motor->setDutyCycle;
        }
        else {
            if (motor->currentDutyCycle < motor->setDutyCycle) motor->currentDutyCycle++;
            else motor->currentDutyCycle--;
        }
        DEBUG_PRINT("updated currentDutyCycle to: %.2f\n", motor->currentDutyCycle);
        motor->pwmOnCycles = motor->period * motor->currentDutyCycle / 100U;
        motor->pwmOffCycles = motor->period - motor->pwmOnCycles;
        SM_word = (motor->pwmOffCycles & 0x7fff) << 17; //shift the 15 bit off-cycles to the MSBs of 32 bit word
        SM_word |= (motor->pwmOnCycles & 0x7fff) << 2; //shift 15 bits of on-cycles next, leaving two bits for dir and run
        SM_word |=  dir + 1; //0b10 or 0b01 will set the out-pins in a single instruction
        motor->runFlag = true;
        motor->dir = dir;
        // DEBUG_PRINT("RUNNING MOTOR\n");
        pio_sm_drain_tx_fifo(motor->PIO_NUM, motor->SM_NUM); //safe
        pio_sm_put_blocking(motor->PIO_NUM, motor->SM_NUM, SM_word);//send our word to the TX FIFO
        // pio_sm_set_enabled(motor->PIO_NUM, motor->SM_NUM, true);//start the SM
        DEBUG_PRINT("Instruction sent to PIO motor controller\n");
        gpio_put(motor->R_ENABLE_PIN, true);
        gpio_put(motor->L_ENABLE_PIN, true);
        // DEBUG_PRINT("SLEEPING FOR 100ms\n");
        sleep_ms(100);
    }
    
}