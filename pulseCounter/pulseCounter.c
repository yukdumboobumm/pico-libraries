//#define DEBUG
#include "pulseCounter.h"

//interrupt usage table
bool intnum_used[INT_NUM_SIZE] = {false};
//interrupt flags
volatile bool intFlags[INT_NUM_SIZE] = {false};
//pulse counter pointer array, used by the ISR
PULSE_COUNTER* pulse_counters[INT_NUM_SIZE] = {NULL};
uint pulseInstances = 0; //keep track (again, for the ISR...this is getting cumbersome)

void initPulseCounter(PULSE_COUNTER *pc, uint pin,  enum INT_NUM intNum) {
    pulseInstances++;
    pulse_counters[intNum] = pc;
    pc->sensorPin = pin;
    pc->count = 0;
    pc->countStartTime = nil_time;
    pc->countEndTime = nil_time;
    pc->timeDiff_us = 0;
    pc->countStarted = false;
    pc->countReady = false;
    pc->intFlag = false;
    if (intnum_used[intNum]==false) {
        pc->interruptNum = intNum;
        intnum_used[intNum] = true;
        switch(intNum) {
            case INT0:
                pc->PIO_IRQ = pis_interrupt0;
                pc->NVIC_IRQ = PIO0_IRQ_0;
                break;
            case INT1:
                pc->PIO_IRQ = pis_interrupt1;
                pc->NVIC_IRQ = PIO0_IRQ_1;
                break;
            case INT2:
                pc->PIO_IRQ = pis_interrupt2;
                pc->NVIC_IRQ = PIO1_IRQ_0;
                break;
            case INT3:
                pc->PIO_IRQ = pis_interrupt3;
                pc->NVIC_IRQ = PIO1_IRQ_1;
                break;
            default:
                DEBUG_PRINT("bad interrupt value. program terminated\n");
                while(1){};
        }
    }
    else { 
        DEBUG_PRINT("interrupt alread used. program terminated\n");
        while(1){};
    }
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);  // set the pin to be an input pullup
    // gpio_pull_down(pin);
    // gpio_pull_up(pin);
}
//this is probably too much work for an ISR. need to rethink how to build this for 
//the possiblity of several pulsecounter instances. will work for the single one i have now.
void generalISRhandler() {
    absolute_time_t thisIntTime = get_absolute_time();
    DEBUG_PRINT("!!in handler!!\n");
    for (int i = 0; i < pulseInstances; i++) {
        if(pulse_counters[i] == NULL) break;
        else if(pio_interrupt_get(pulse_counters[i]->PIO_NUM, pulse_counters[i]->interruptNum)) {
            pio_interrupt_clear(pulse_counters[i]->PIO_NUM, pulse_counters[i]->interruptNum);
            pulse_counters[i]->countEndTime = thisIntTime; 
            pulse_counters[i]->intFlag = true;
            pulse_counters[i]->countStarted = false;
            return;
        }
    }
    DEBUG_PRINT("no interrupt found\n");
}

//grab a state machine, load, and initialize it
//need a better mechanism (and understanding) for how to grab an interrupt number
//like wtf is irq0 vs irq1. yeesh.
void initPulsePIO(PULSE_COUNTER *pc, PIO pionum) {
    pc->PIO_NUM = pionum;
    pc->SM_NUM = pio_claim_unused_sm(pc->PIO_NUM, true);// Get first free state machine in selected PIO
    pc->OFFSET = pio_add_program(pc->PIO_NUM, &pulse_counter_program);// Add PIO program to PIO instruction memory, return memory offset
    pulse_counter_program_init(pc->PIO_NUM, pc->SM_NUM, pc->OFFSET, pc->sensorPin);//initialize the SM
    pio_sm_set_enabled(pc->PIO_NUM, pc->SM_NUM, true);//start the SM
    DEBUG_PRINT("Loaded PIO program at %d and started SM: %d\n", pc->OFFSET, pc->SM_NUM);
    irq_set_exclusive_handler(pc->NVIC_IRQ, generalISRhandler);
	irq_set_enabled(pc->NVIC_IRQ, true); //enable interrupt for NVIC
    pio_set_irq0_source_enabled(pionum, pc->PIO_IRQ, true);
    DEBUG_PRINT("finished setting interrupt\n");
}

void startCount(PULSE_COUNTER *pc, uint numPulses) {
    DEBUG_PRINT("sending start instruction\n");
    if (pc->countStarted == false) {
        pc->countStartTime = get_absolute_time();
        pio_sm_put_blocking(pc->PIO_NUM, pc->SM_NUM, numPulses);//send a word to the TX FIFO, blocking if full
        pc->countStarted = true;
        pc->count = numPulses;
    }
    DEBUG_PRINT("%d counts requested\n", numPulses);
}

void calcSpeed(PULSE_COUNTER *pc) {
    pc->timeDiff_us = absolute_time_diff_us(pc->countStartTime, pc->countEndTime);
    int32_t timeDiff_ms = pc->timeDiff_us / 1000;
    int32_t timeDiff_s = timeDiff_ms / 1000;
    if (pc->timeDiff_us) {
        DEBUG_PRINT("count: %d time diff us: %llu ms: %d secs: %d\n", pc->count, pc->timeDiff_us, timeDiff_ms, timeDiff_s);
        pc->currentHertz = ((double)pc->count * 1000 * 1000) / (pc->timeDiff_us);
        pc->currentRPM = pc->currentHertz * 60;
        DEBUG_PRINT("%.2f Hz, %d RPM\n", pc->currentHertz, (uint)pc->currentRPM);
    }
    else DEBUG_PRINT("no time difference. Skipping...\n");
}

inline void clearFlag(PULSE_COUNTER *pc) {
    pc->intFlag = false;
}

inline bool checkFlag(PULSE_COUNTER *pc) {
    return pc->intFlag;
}