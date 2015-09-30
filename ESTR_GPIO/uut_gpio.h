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
#include "drivers/rit128x96x4.h"

#include "pins.h"
#include "stopwatch.h"


// CONFIG Parameters
#ifndef CONFIG_H_	// Once merged with UART module, test params will be set in a config file

	#define TEST_ONE_MAX_PERIOD 600  //undefined unit
	#define TEST_ONE_MIN_PERIOD 300   //undefined unit
	#define TEST_ONE_FREQ_STEP 100

	#define TEST_ONE_NUM_PULSES 2000	//currently controls the number of pulses for all tests
	#define TEST_ONE_MAX_RESPONSE_TIME_US 800 //TODO: remove once remote proccessing is in place

	#define TEST_TWO_INIT_PERIOD 600
	#define TEST_TWO_MIN_PERIOD 100
	#define TEST_TWO_FREQ_STEP 50

	#define TEST_THREE_PERIOD 600

	#define TEST_FOUR_PERIOD_A 600
	#define TEST_FOUR_PERIOD_B 599

	#define TEST_FIVE_PERIOD 600

#endif


void airspeed_response_isr(void);
void transponder_response_isr(void);

// PWM output ISRS
void airspeed_pulse_isr(void);
void transponder_pulse_isr(void);

int test_b_output_toggle(void); // called by the below isr to toggle pwm output
void airspeed_pulse_isr_gpio_test_b(void); // modified airspeed isr for test b

void test_one_frequency_mod(void);
void test_two_frequency_mod(void);



#endif /* TEST_ONE_H_ */
