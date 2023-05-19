#ifndef PULSECOUNTER_H
#define PULSECOUNTER_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pulseCounter.pio.h"

//DEFINES
#ifdef DEBUG
    #ifndef DEBUG_PRINT
        #define DEBUG_PRINT(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
    #endif
#else
    #define DEBUG_PRINT(fmt, ...) (void)0
#endif

//forward declaration of struct
typedef struct PULSE_COUNTER PULSE_COUNTER;
// interrupt time holder
extern absolute_time_t intTime;
//global variables:


//PULSE_COUNTER "object"
struct PULSE_COUNTER {
    uint sensorPin;
    
    uint32_t count;
    absolute_time_t countStartTime;
    absolute_time_t countEndTime;
    int64_t timeDiff_us;

    bool countStarted;
    bool countReady;

    float currentHertz;
    float currentRPM;

    bool intFlag;
    uint interruptNum;
    uint PIO_IRQ;
    uint NVIC_IRQ;
    
    PIO PIO_NUM;
    uint SM_NUM;
    uint OFFSET;
};
//convenience holder for interrupt names
enum INT_NUM{
    INT0,
    INT1,
    INT2,
    INT3,
    INT_NUM_SIZE
};
//interrupt usage table
extern bool intnum_used[INT_NUM_SIZE];
//interrupt flags
extern volatile bool intFlags[INT_NUM_SIZE];

//isr handler
void generalISRhandler(void);
//function prototypes
void initPulseCounter(PULSE_COUNTER *, uint,  enum INT_NUM);
void initPulsePIO(PULSE_COUNTER *, PIO);
void startCount(PULSE_COUNTER *, uint);
// void getCount(PULSE_COUNTER *);
void calcSpeed(PULSE_COUNTER *);
void clearFlag(PULSE_COUNTER *);
bool checkFlag(PULSE_COUNTER *);

#endif // PULSECOUNTER_H