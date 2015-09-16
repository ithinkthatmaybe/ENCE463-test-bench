/*
 * main.c
 *
 *  *  Created on: Sep 13, 2015
 *      Author: Ben Litchfield and Jack Linton
 *      Description: 	Tests the mirror function on the UUT by
 *      				sending a grave character, then the
 *      				string "123" as individual characters,
 *      				followed by another grave. Checks that
 *      				the expected response "ECHO ON123ECHO OFF"
 *      				is received. Displays a pass/fail on
 *      				Stellaris screen or "time out" if no message is received within 1.5 seconds from start-up.  
 
						Furthermore, the program will reset the UUT before testing begins 
						if pin G4 is connected to the reset pin on the uut ASP connector. 
 *
 */

/* FreeRTOS includes. */
#include "include/FreeRTOS.h"
#include "include/task.h"
#include "include/queue.h"

/* Stellaris library includes. */
#include "inc/lm3s1968.h"
#include "inc\hw_types.h"
#include "inc\hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "drivers/rit128x96x4.h"
#include "stdio.h"
#include "inc/hw_ints.h"
#include "string.h"



/* Demo includes. */
#include "demo_code\basic_io.h"

/* Other Module Includes */
#include "UART.h"
#include "stopwatch.h"
#include "uut_gpio.h"

/* Used as a loop counter to create a very crude delay. */
#define mainDELAY_LOOP_COUNT		( 0xfffff )

#define configUSE_16_BIT_TICKS 0

#define INCLUDE_vTaskDelay 1
#define MIRROR_START_STOP "`"
#define MIRROR "123"
#define EXPECTED_RESPONSE "ECHO ON123ECHO OFF"
#define RESPONSE_LENGTH sizeof(EXPECTED_RESPONSE)/sizeof(char)-1
#define MIRROR_LENGTH sizeof(MIRROR)/sizeof(char)-1
//#define NO_MIRROR "456"

/* The task function. */
void vMirrorTX( void );
void vSend( void );
void vTimeout( void );
/* Define the strings that will be passed in as the task parameters.  These are
defined const and off the stack to ensure they remain valid when the tasks are
executing. */

/*-----------------------------------------------------------*/



int main( void )
{
	
	
	// ALL INIT calls will be added to an init module at a later date. 
	
	/* Set the clocking to run from the PLL at 50 MHz.  Assumes 8MHz XTAL,
	whereas some older eval boards used 6MHz. */
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
	
	IntMasterEnable();
    // Initialize the OLED display and write status.
    RIT128x96x4Init(1000000);
    //RIT128x96x4StringDraw("UART Send",            36,  0, 15);
    RIT128x96x4StringDraw("UART Mirror",            36,  0, 15);

    //Module initialisation functions.
    InitUART();
    InitGPIO ();

	// Create required tasks. First task creation has comments for ease of use.
	xTaskCreate(	vMirrorTX,			/* Pointer to the function that implements the task. */
					"Task 1",				/* Text name for the task.  This is to facilitate debugging only. */
					240,					/* Stack depth in words. */
					NULL,	/* Pass the text to be printed in as the task parameter. */
					2,						/* This task will run at priority 1. */
					NULL );					/* We are not using the task handle. */

	xTaskCreate(	vSend, "Task 2", 240, NULL, 2, NULL );
	xTaskCreate(	vTimeout, "Task 3", 240, NULL, 2, NULL );

	//Start the scheduler so the tasks start executing.
	vTaskStartScheduler();	
	
	for( ;; );
}
/*-----------------------------------------------------------*/

xQueueHandle xToTimeout;
xQueueHandle xToMirror;

void vTimeout( void )
{

	stopwatch_t stopwatch;  // instantiates a stopwatch timer 
	unsigned long time = 0;
	char cReceived;
	int finished = 0;
	char strbuff[20];

	while (1)
	{
		if (xToTimeout !=0)
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				//Sending is finished, start time out timer
				if (cReceived == 'S'){
					break;
				} else {
					sprintf(strbuff,"ERROR STOPWATCH");
					RIT128x96x4StringDraw(strbuff, 5, 20, 30);
				}
			}
		}
	}

	stopwatch_start(&stopwatch); 

	for( ;; )
	{
		vTaskDelay(1);
		stopwatch_stop(&stopwatch);
		time = stopwatch_get_time_ms(&stopwatch);
		if (time >= 500 && !finished){
			sprintf(strbuff,"Fail timeout");
			RIT128x96x4StringDraw(strbuff, 5, 20, 30);
		}
		if (xToTimeout !=0)  
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				//Sending is finished, start time out timer
				if (cReceived == 'F'){
					finished = 1;
				}
			}
		}
	}
}

void vSend( void )
{
	
	char messageSent = 'S';
	xToTimeout = xQueueCreate(2, sizeof(char)); // sets up queue to pass message to time out task. 
	vTaskDelay(500);
	UARTSend((unsigned char *)MIRROR_START_STOP, sizeof(char));
	vTaskDelay(200);
	UARTSend((unsigned char *)MIRROR, MIRROR_LENGTH);
	vTaskDelay(200);
	UARTSend((unsigned char *)MIRROR_START_STOP, sizeof(char)); // sends message to start time out timer via queue

	xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

	for( ;; )
	{
		vTaskDelay(1);    // Present to stop task from ending. 
	}
}

void vMirrorTX( void )

{ 	//  reads mirrored UART characters from UUT and compares them to the charters originally sent to the UUT.
    //  Magic numbers to be replaced with hash defines in header file. Task also displays results on the stellaris screen
	char cReceived;
	char strbuff[20]; 
	int i = 0;
	char buffer[18] = {0};

	char finished = 'F';

	for( ;; )

	{
		if (xUARTReadQueue !=0)
		{
			if (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
			{
				buffer[i++] = cReceived;
				if (i == RESPONSE_LENGTH){
					if (strncmp(buffer, EXPECTED_RESPONSE, 18)==0){   // Compares mirrored message to sent message. 
						sprintf(strbuff,"Pass");
						RIT128x96x4StringDraw(strbuff, 5, 10, 30);
						xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
						break;
					} else {
						sprintf(strbuff,"Fail wrong received");
						RIT128x96x4StringDraw(strbuff, 5, 10, 30);
						xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
						break;
					}
				}
			}
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


