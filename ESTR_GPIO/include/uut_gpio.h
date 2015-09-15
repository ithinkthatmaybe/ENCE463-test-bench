/*
 * uut_gpio.h
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */

#ifndef UUT_GPIO_H_
#define UUT_GPIO_H_

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "inc\hw_memmap.h"
#include "inc\hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "drivers/rit128x96x4.h"

#include "../pins.h"
#include "stopwatch.h"


// Parameters
#define TEST_ONE_INIT_PERIOD 3000  //undefined unit
#define TEST_ONE_MIN_PERIOD 1500   //undefined unit
#define TEST_ONE_NUM_CYCLES 3
#define TEST_ONE_MAX_RESPONSE_TIME_US 400


// Globals
stopwatch_t g_airspeed_stopwatch;
char g_airspeed_response_flag;

// functions

void vISRgpio_port_e(void);

void vISRgpio_port_c(void);

void uut_gpio_init(void);

void vTaskStimulateAirspeed(void *pvParameters);

#endif /* UUT_GPIO_H_ */
