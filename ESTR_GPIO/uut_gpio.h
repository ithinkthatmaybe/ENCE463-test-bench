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
#define TEST_ONE_INIT_PERIOD 3000  //undefined unit
#define TEST_ONE_MIN_PERIOD 1500   //undefined unit
//#define TEST_ONE_NUM_CYCLES 3
#define TEST_ONE_NUM_PULSES 200
#define TEST_ONE_MAX_RESPONSE_TIME_US 400






// functions
void vTaskDisplayResults(void);

void airspeed_pulse_generation(void);

void transponder_pulse_generation(void);

void vISRgpio_port_e(void);

void vISRgpio_port_c(void);

void uut_gpio_init(void);

void vTaskStimulateAirspeed(void *pvParameters);

#endif /* TEST_ONE_H_ */
