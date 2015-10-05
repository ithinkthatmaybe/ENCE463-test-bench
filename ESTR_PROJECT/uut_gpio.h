/*
 * uut_gpio.h
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */

#ifndef TEST_ONE_H
#define TEST_ONE_H

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


// CONFIG Parameters
//#ifndef CONFIG_H_
//
//		//currently controls the number of pulses for all tests
//
//	#define TEST_ONE_MAX_PERIOD_US 600  //undefined unit
//	#define TEST_ONE_MIN_PERIOD_US 300   //undefined unit
//	#define TEST_ONE_FREQ_STEP_US 100
//
//	#define TEST_TWO_INIT_PERIOD_US 600
//	#define TEST_TWO_MIN_PERIOD_US 100
//	#define TEST_TWO_FREQ_STEP_US 50
//
//	#define TEST_THREE_PERIOD_US 600
//
//	#define TEST_FOUR_PERIOD_US_A 600
//	#define TEST_FOUR_PERIOD_US_B 599
//
//	#define TEST_FIVE_PERIOD_MS 30
//	#define TEST_FIVE_NUM_PULSES 5
//
//#endif



// set up GPIO pins on stellaris
void InitGPIO (void);

// Reset UUT (active low)
void reset_uut(void);

// Globals
extern int g_num_pulses;

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



#endif /* TEST_ONE_H_ */
