/*
 * uut_gpio.h
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */

#ifndef UUT_GPIO_H
#define UUT_GPIO_H

#include <stdio.h>

#include "include/FreeRTOS.h"
#include "include/task.h"

#include "inc\hw_memmap.h"
#include "inc\hw_types.h"
#include "inc\hw_pwm.h"

#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"

#include "pins.h"
#include "stopwatch.h"
#include "PC_UART.h"

#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)
#define CYCLES_PER_MS (configCPU_CLOCK_HZ/1E3)

#define MAX_NUM_PULSES 200


// set up GPIO pins on stellaris
void InitGPIO (void);

// Reset UUT (active low)
void reset_uut(void);

// Globals
extern int g_num_pulses;

// TODO: 
extern int g_airspeed_pulse_count;
extern int g_airspeed_response_flags[MAX_NUM_PULSES];
extern unsigned int g_airspeed_times[MAX_NUM_PULSES];

extern int g_transponder_pulse_count;
extern int g_transponder_response_flags[MAX_NUM_PULSES];
extern unsigned int g_transponder_times[MAX_NUM_PULSES];


void uut_gpio_set_num_pulses(int pulses);

// PWM output ISRS
void airspeed_pulse_isr(void);
void transponder_pulse_isr(void);

void airspeed_response_isr(void);
void transponder_response_isr(void);


int test_b_output_toggle(void); // called by the below isr to toggle pwm output
void airspeed_pulse_isr_gpio_test_b(void); // modified airspeed isr for test b

void test_one_frequency_mod(void);
void test_two_frequency_mod(void);



#endif /* UUT_GPIO_H_ */
