/*
    FreeRTOS V6.0.5 - Copyright (C) 2009 Real Time Engineers Ltd.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#include <stdio.h>

/* FreeRTOS includes. */
#include "include/FreeRTOS.h"
#include "include/task.h"

/* Stellaris library includes. */
#include "inc\hw_types.h"
#include "inc\hw_memmap.h"
//#include "inc\hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h" //to read systick register
#include "driverlib/gpio.h"
#include "driverlib/interrupt.c"

#include "drivers/rit128x96x4.h"

/* specific includes */
#include "pins.h"

#include "include/stopwatch.h"
#include "test_one.h"

/* Used as a loop counter to create a very crude delay. */
#define mainDELAY_LOOP_COUNT		( 0xfffff )


// prototypes
void vTaskHeartbeat( void *pvParameters );


/*-----------------------------------------------------------*/



int main( void )
{
	IntMasterDisable();

	/* Set the clocking to run from the PLL at 50 MHz.  Assumes 8MHz XTAL,
	whereas some older eval boards used 6MHz. */
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );

	// Setup ESTR peripherals and register test tasks and ISRs
	uut_gpio_init();

	// Create ESTR tasks
	xTaskCreate( vTaskHeartbeat, "Heartbeat", 240, NULL, 1, NULL);


	//NOTE: code acts in in place of python test manager, will be removed.
	RIT128x96x4Init(1000000);
	RIT128x96x4StringDraw("Press select to ", 8, 0, 4);
	RIT128x96x4StringDraw("start test", 8, 12, 4);
	GPIOPinTypeGPIOInput(GPIO_PORTG_BASE, (1<<7));
	while(GPIOPinRead(GPIO_PORTG_BASE, (1<<7)) == (1<<7)); // wait for button press
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("running... ", 8, 0, 4);


	/* Start the scheduler so our tasks start executing. */
	IntMasterEnable();
	vTaskStartScheduler();	
	
	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/


void vTaskHeartbeat(void *pvParameters)
{
	int iTaskDelayPeriod = 500 / portTICK_RATE_MS;		// TODO: macro
	for (;;)
	{
		GPIOPinWrite(GPIO_PORTG_BASE, STATUS_LED_PG2, STATUS_LED_PG2);
		vTaskDelay(iTaskDelayPeriod);
		GPIOPinWrite(GPIO_PORTG_BASE, STATUS_LED_PG2, ~STATUS_LED_PG2);
		vTaskDelay(iTaskDelayPeriod);
	}
}


/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* This function will only be called if an API call to create a task, queue
	or semaphore fails because there is too little heap RAM remaining. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
	/* This function will only be called if a task overflows its stack.  Note
	that stack overflow checking does slow down the context switch
	implementation. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* contemplate existance */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This example does not use the tick hook to perform any processing. */
}


