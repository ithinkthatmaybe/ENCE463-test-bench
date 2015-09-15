/*
 * uut_gpio.h
 *
 *  Created on: Sep 15, 2015
 *      Author: jwl52
 */

#ifndef UUT_GPIO_H_
#define UUT_GPIO_H_

/* Stellaris library includes. */
#include "inc/lm3s1968.h"
#include "inc\hw_types.h"
#include "inc\hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "stdio.h"


// set up GPIO pins on stellaris
void InitGPIO (void);

// Reset UUT (active low)
void reset_uut(void);


#endif /* UUT_GPIO_H_ */
