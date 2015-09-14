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

/* Demo includes. */
#include "demo_code\basic_io.h"

/* specific includes */
#include "pins.h"

#include "include/stopwatch.h"

/* Used as a loop counter to create a very crude delay. */
#define mainDELAY_LOOP_COUNT		( 0xfffff )

/* The task function. */
void vTaskHeartbeat( void *pvParameters );
void vTaskStimulateAirspeed( void *pvParameters);
void vTaskTestStopwatch(void);

// isr prototype
void vISRgpio_port_e(void);

/* Define the strings that will be passed in as the task parameters.  These are
defined const and off the stack to ensure they remain valid when the tasks are
executing. */
const char *pcTextForTask1 = "Task 1 is running\n";
const char *pcTextForTask2 = "Task 2 is running\n";

/*-----------------------------------------------------------*/

stopwatch_t g_airspeed_stopwatch;

int main( void )
{
	IntMasterDisable();

	// GPIO init / signs of life
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, STATUS_LED_PG2);
	GPIOPinWrite(GPIO_PORTG_BASE, STATUS_LED_PG2, STATUS_LED_PG2);


	/* Set the clocking to run from the PLL at 50 MHz.  Assumes 8MHz XTAL,
	whereas some older eval boards used 6MHz. */
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );


	// Setup Airspeed response ISR
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &vISRgpio_port_e);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Setup Airspeed output
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2);

	// Create ESTR tasks
	xTaskCreate( vTaskHeartbeat, "Heartbeat", 240, NULL, 1, NULL);
	xTaskCreate( vTaskStimulateAirspeed, "Airspeed Stimulus", 240, NULL, 1, NULL);
//	xTaskCreate( vTaskTestStopwatch, "stopwatch test", 240, NULL, 1, NULL);

	/* Start the scheduler so our tasks start executing. */
	IntMasterEnable();
	vTaskStartScheduler();	
	
	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vTaskTestStopwatch(void)
{
	stopwatch_t stopwatch;
	for(;;)
	{
		stopwatch_start(&stopwatch);
		vTaskDelay(300 / portTICK_RATE_MS);
		stopwatch_stop(&stopwatch);
//		vTaskDelay(300);
		unsigned long time = stopwatch_get_time_ms(&stopwatch);
	}
}

// ISR responds to airspeed pulses
void vISRgpio_port_e(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//	GPIOPinWrite(GPIO_PORTG_BASE, STATUS_LED_PG2, STATUS_LED_PG2);
	GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);

	stopwatch_stop(&g_airspeed_stopwatch);
	return;
}

// ISR responds to transponder & kernel interrupts
void vISRgpio_port_c(void)
{
	return;
}


//TODO: Variable width, find minimal detection width
//TODO: Variable frequency, finid maximum operating frequency, or what percentage of total pulses are missed as a function of frequency
//TODO: measure average, maximum response time
void vTaskStimulateAirspeed(void *pvParameters)
{
//	int iTaskDelayPeriod = 20 / portTICK_RATE_MS;		// TODO: macro

	static unsigned long average_response_time = 0;
	static unsigned long max_response_time = 0;

	static unsigned char initialised = 0;


	for (;;)
	{
		initialised = 0;	//Not writing, wierd
		max_response_time = 0;
		average_response_time = 0;

		unsigned long j;
		for (j = 0; j < 200; j++)
		{

			if (initialised == 0)
				initialised++;
			else if (initialised == 1){
				average_response_time = stopwatch_get_time_us(&g_airspeed_stopwatch);
				max_response_time = average_response_time;
				initialised++;
			}
			else{
				unsigned long time = stopwatch_get_time_us(&g_airspeed_stopwatch); // check the stopwatch from the last stimulation
				average_response_time = (average_response_time+time)/2;
				if (time > max_response_time)
					max_response_time = time;
			}


			stopwatch_start(&g_airspeed_stopwatch);			// TODO: protect stopwatch with mutex
			GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, UUT_AIRSPEED_OUTPUT_PIN_PE2);
		//		vTaskDelay(iTaskDelayPeriod/2);
			unsigned int i;
			for (i = 0; i < 2000; i++)
				continue;

			// this shouldnt do anything, but stops the pin state from getting stuck high if the
			// UUT throws a wobbly or something
		//		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);
		//		vTaskDelay(iTaskDelayPeriod/2);
		}
		char breakpoint = 0;
	}
}

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



void vTaskFunction( void *pvParameters )
{
char *pcTaskName;
volatile unsigned long ul;

	/* The string to print out is passed in via the parameter.  Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{
		/* Print out the name of this task. */
		vPrintString( pcTaskName );

		/* Delay for a period. */
		for( ul = 0; ul < mainDELAY_LOOP_COUNT; ul++ )
		{
			/* This loop is just a very crude delay implementation.  There is
			nothing to do in here.  Later exercises will replace this crude
			loop with a proper delay/sleep function. */
		}
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
	/* This example does not use the idle hook to perform any processing. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This example does not use the tick hook to perform any processing. */
}


