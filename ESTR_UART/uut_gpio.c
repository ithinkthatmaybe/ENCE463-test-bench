/*
 * uut_gpio.c
 *
 *  Created on: Sep 15, 2015
 *      Author: jwl52
 */

#include "uut_gpio.h"


void InitGPIO (void)
{
	// Enable GPIO port G
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_4);
	GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_PIN_4);
	reset_uut();
}



void reset_uut(void){
	 GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, 0);
	 int i = 0;
	 for ( i=0 ; i >100000 ;i++)
	 {continue;}
	 GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_PIN_4);
	 for ( i=0 ; i >100000 ;i++)
	 {continue;}
}
