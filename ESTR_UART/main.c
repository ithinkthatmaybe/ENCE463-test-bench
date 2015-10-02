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
 *      				Stellaris screen.
 *
 */

// FreeRTOS includes.
#include "include/FreeRTOS.h"
#include "include/task.h"
#include "include/queue.h"
#include "include/semphr.h"

// Stellaris library includes.
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
#include "stdlib.h"

// Test module includes.
#include "UART.h"
#include "PC_UART.h"
#include "stopwatch.h"
#include "uut_gpio.h"

// These two definitions change parameters in FreeRTOS.
#define configUSE_16_BIT_TICKS 0
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelete 1

// Messages to be sent for the test.
#define MIRROR_START_STOP "`"
#define MIRROR "`123`456"
#define NO_MIRROR "456"
//#define EXPECTED_RESPONSE "ECHO ON123ECHO OFFNACKNACKNACK"
#define EXPECTED_RESPONSE "ECHO ON123ECHO OFFECHO ON123ECHO OFFECHO ON123ECHO OFFECHO ON123ECHO OFF"
#define EXPECTED_RESPONSE_LENGTH strlen(EXPECTED_RESPONSE)
#define MIRROR_LENGTH strlen(MIRROR)
#define NO_MIRROR_LENGTH strlen(NO_MIRROR)

// Tests:
// 1 Mirror TX
// 2 Processor Clock Variation
int gTest = 3;

// The task functions.
void vMirrorTX( void );
xTaskHandle xMirrorTX;
void vClockSpeed( void );
xTaskHandle xClockSpeed;
void vIntercept( void );
xTaskHandle xIntercept;

void vTimeout( void );
/*-----------------------------------------------------------*/

int main( void )
 {
	// Set the clocking to run from the PLL at 50 MHz.  Assumes 8MHz XTAL,
	// whereas some older eval boards used 6MHz.
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
	IntMasterEnable();
    // Initialize the OLED display and write status.
    RIT128x96x4Init(1000000);
    RIT128x96x4StringDraw("UART Mirror", 36, 0, 15);

    // Module initialisation functions.
    InitUART();
    InitGPIO ();
    Init_PC_UART();

    //  The first character appears to be ignored by the UUT, so a single character is sent before the
    //  input UART starts to be read.
    UARTSend((unsigned char *)"q", 1, UART1_BASE);

	// Create required tasks. First task creation has comments for ease of use.

    xTaskCreate(Monitor_PC_UART, "Task 5", 240, NULL, 1, NULL );
//
//    // vMirrorTX is test 1a.
//    if (gTest == 1){
//    	xTaskCreate(	vMirrorTX,			/* Pointer to the function that implements the task. */
//    						"Task 1",				/* Text name for the task.  This is to facilitate debugging only. */
//    						240,					/* Stack depth in words. */
//    						NULL,	/* Pass the text to be printed in as the task parameter. */
//    						5,						/* This task will run at priority 1. */
//    						NULL );					/* We are not using the task handle. */
//    } else if (gTest == 2){
//    	// vClockSpeed is test 1d.
//    	xTaskCreate(vClockSpeed, "Task 2", 240, NULL, 2, NULL );
//    	xTaskCreate(vIntercept, "Task 3", 240, NULL, 2, NULL );
//    }




	// vTimeout ends the test after the timeout period is completed, usually because an error
	// occurred and the UUT is not responding to the input.
	xTaskCreate(vTimeout, "Task 4", 240, NULL, 2, NULL );

	// Start the scheduler so the tasks start executing.
	vTaskStartScheduler();	
	
	// If all is well we will never reach here as the scheduler will now be
	// running.  If we do reach here then it is likely that there was insufficient
	// heap available for the idle task to be created.
	for( ;; );
}

/*-----------------------------------------------------------*/
xQueueHandle xToMonPC;
xSemaphoreHandle xTestMutex;

void Monitor_PC_UART()
{
	// Monitors the  PC receive queue (UART chanel 0) and prints 10 characters on the screen of the stellaris.
	// If more than 10 chars are received the screen is reset and printing begins again.
	char cReceived;
	xToMonPC = xQueueCreate(5, sizeof(char));
	xTestMutex = xSemaphoreCreateMutex();

	typedef enum states {IDLE, TASK1, TASK2} CurrState;
	CurrState state = IDLE;

	xSemaphoreTake (xPC_SENT, (portTickType)100);

	if (xTestMutex == NULL)
	{
		RIT128x96x4StringDraw("Mutex issue", 5, 20, 30);
	}
	for( ;; )
	{
		if(state == IDLE)
		{
			if (xCOMMS_FROM_PC_Queue !=0)
			{
				if (xQueueReceive(xCOMMS_FROM_PC_Queue, &cReceived, (portTickType)10))
				{
					if (cReceived == '1')
					{
						xTaskCreate(vMirrorTX, "Task 1", 240, NULL, 5, &xMirrorTX );
						state = TASK1;
					} else if (cReceived == '2')
					{
						xTaskCreate(vClockSpeed, "Task 2", 240, NULL, 2, &xClockSpeed );
						xTaskCreate(vIntercept, "Task 3", 240, NULL, 2, &xIntercept );
						state = TASK2;
					} else
					{
						RIT128x96x4StringDraw("Invalid test number", 5, 20, 30);
					}
					//xTaskCreate(vTimeout, "Task 4", 240, NULL, 2, NULL );
				}

			}
		} else if (state == TASK1)
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				vTaskDelete(xMirrorTX);
				state = IDLE;
			}
		} else if (state == TASK2)
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				vTaskDelete(xClockSpeed);
				vTaskDelete(xIntercept);
				state = IDLE;
			}
		}

	}
}

/*-----------------------------------------------------------*/

xQueueHandle xToTest;
xQueueHandle xToTimeout;

void vTimeout( void )
{
	xToTest = xQueueCreate(5, sizeof(char));
	stopwatch_t stopwatch;
	unsigned long time = 0;
	char cReceived;
	int finished = 0;
	char strbuff[20];
	// Start stopwatch in stopped mode.
	int stopped = 1;
	int timeout = 200;
	char fail = 'F';
	vTaskDelay(1);
	for( ;; )
	{
		if (xToTimeout !=0 && stopped == 1)
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				//Sending is finished, start timeout timer
				if (cReceived == 'S'){
					stopped = 0;
					stopwatch_start(&stopwatch);
				} else {
					sprintf(strbuff,"ERROR STOPWATCH");
					RIT128x96x4StringDraw(strbuff, 5, 20, 30);
				}
			}
		} else if (stopped == 0){
			stopwatch_stop(&stopwatch);
			time = stopwatch_get_time_ms(&stopwatch);
		}
		vTaskDelay(1);
		if (time >=timeout && !finished){
			//sprintf(strbuff,"Fail timeout");
			//RIT128x96x4StringDraw(strbuff, 5, 20, 30);
			xQueueSendToBack( xToTest, &fail, (portTickType)10);
		}
		if (xToTimeout !=0 && stopped == 0)
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				//Sending is finished, start timeout timer
				if (cReceived == 'F'){
					finished = 1;
				} else if (cReceived == 'E'){
					stopped = 1;
					time = 0;
				}
			}
		}
	}
}

void vIntercept( void )
{
	char fail = '|';
	char cReceived;
	for ( ;; )
	{
		if (xToTest !=0)
		{
			while (xQueueReceive(xToTest, &cReceived, (portTickType)10))
			{
				if (cReceived == 'F')
				{
					xQueueSendToBack( xUARTReadQueue, &fail, (portTickType)10);
				}
			}
		}
	}

}

void vMirrorTX( void )
{
	// SET THESE UP TO BE RECEIVED FROM PC
	unsigned char mirror[50] = "`123`456";
	int len = strlen(mirror);
	char expect[50] = "ECHO ON123ECHO OFFNACKNACKNACK";
	int expectedLen = strlen(expect);
	char pass[5] = "Pass";
	char fail[5] = "Fail";
	// Variables
	char strbuff[20] = {0};
	char * buffer = (char *) malloc (expectedLen);

	char messageSent = 'S';
	char finished = 'F';
	int done = 0;
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '1';
	results_ptr = &results;

	xToTimeout = xQueueCreate(2, sizeof(char));
	xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);
	vTaskDelay(100);
	// Clears the queue due to an issue where an 'l' was in the queue whne initialized.
	char cReceived;
	if (xUARTReadQueue !=0)
	{
		while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
		{
			cReceived = 0;
		}
	}

	for( ;; )
	{

		if (xToTest !=0)
		{
			while (xQueueReceive(xToTest, &cReceived, (portTickType)10))
			{
				if (cReceived == 'F')
				{
					sprintf(strbuff,"Fail timeout");
					RIT128x96x4StringDraw(strbuff, 5, 20, 30);
				}
			}
		}
		mirrorUART(mirror, len, UART1_BASE, expectedLen, buffer);
		if (strncmp(buffer, expect, expectedLen)==0)
		{

			results.test_string = pass;
			results.test_string_len = strlen(results.test_string);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			sprintf(strbuff,"Expected response");
			RIT128x96x4StringDraw(strbuff, 5, 10, 30);
			xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
			free(buffer);
			done = 1;
		} else
		{
			results.test_string = fail;
			results.test_string_len = strlen(results.test_string);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			sprintf(strbuff,"Fail wrong received");
			RIT128x96x4StringDraw(strbuff, 5, 10, 30);
			xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
			free(buffer);
			done = 1;
		}
		while (done)
		{
			vTaskDelay(100);
		}
	}
}

void vClockSpeed( void )
{
	// SET THESE UP TO BE RECEIVED FROM PC
	unsigned char mirror[10] = "-`123`";
	unsigned char mirror2[10] = "+`123`";
	int len = strlen(mirror);
	char expect[20] = "ECHO ON123ECHO OFF";
	int expectedLen = strlen(expect);

	int increases = 0;
	int decreases = 0;
	// Variables
	char strbuff[20] = {0};
	char * buffer = (char *) malloc (expectedLen);
	int set = 0;
	int i = 0;
	char messageSent = 'S';
	//char finished = 'F';
	char ended = 'E';

	int done = 0;
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '2';
	results_ptr = &results;

	vTaskDelay(100);

	xToTimeout = xQueueCreate(2, sizeof(char));


	// Clears the queue due to an issue where an 'l' was in the queue when initialized.
	char cReceived;
	if (xUARTReadQueue !=0)
	{
		while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
		{
			cReceived = 0;
		}
	}

	for( ;; )
	{
		xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

		if (!set)
		{
			mirrorUART(mirror, len, UART1_BASE, expectedLen, buffer);
		} else {
			mirrorUART(mirror2, len, UART1_BASE, expectedLen, buffer);
		}


		if (strncmp(buffer, expect, expectedLen)==0)
		{
			xQueueSendToBack( xToTimeout, &ended, (portTickType)10);
		} else if (!set)
		{
			xQueueSendToBack( xToTimeout, &ended, (portTickType)10);
			reset_uut();
			vTaskDelay(100);
			decreases = increases;
			increases = 0;
			set = 1;
		} else if (set == 1){
			sprintf(strbuff,"Inc: %d Dec: %d", increases, decreases);
			int data [2]= {0};
			data[0] = increases;
			data[1] = decreases;
			results.test_data = data;
			results.num_of_elements = 2;
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			RIT128x96x4StringDraw(strbuff, 5, 20, 30);
			set = 2;
		} else if (set == 2)
		{
			while (1)
			{
				vTaskDelay(100);
			}
		}
		while (i < expectedLen)
		{
			buffer[i] = 0;
			i++;
		}
		i = 0;
		increases++;
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


