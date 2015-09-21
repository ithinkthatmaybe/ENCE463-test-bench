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

#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "drivers/rit128x96x4.h"

#include "pins.h"
#include "stopwatch.h"


// Parameters
#define TEST_ONE_MAX_PERIOD 600  //undefined unit
#define TEST_ONE_MIN_PERIOD 300   //undefined unit
#define TEST_ONE_FREQ_STEP 20
//#define TEST_ONE_NUM_CYCLES 3
#define TEST_ONE_NUM_PULSES 5000
#define TEST_ONE_MAX_RESPONSE_TIME_US 400

#define TEST_TWO_INIT_PERIOD 600
#define TEST_TWO_MIN_PERIOD 100
#define TEST_TWO_FREQ_STEP 20

#define TEST_THREE_PERIOD 600



// functions
void test_one_pulse_gen_isr(void);
void test_two_pulse_gen_isr(void);
void test_three_pulse_gen_isr(void);
void test_four_pulse_gen_isr_A(void);
void test_four_pulse_gen_isr_B(void);

void transponder_pulse_generation(void);
void airspeed_pulse_generation(void);

void airspeed_response_isr(void);
void transponder_response_isr(void);

void vTaskDisplayResults(void);

void uut_gpio_init(void);

void uut_gpio_test_one_init(void);
void uut_gpio_test_two_init(void);
void uut_gpio_test_three_init(void);

#endif /* TEST_ONE_H_ */
