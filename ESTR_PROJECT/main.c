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
#include "test.h"
#include "UART.h"
#include "PC_UART.h"
#include "stopwatch.h"
#include "uut_gpio.h"

// These two definitions change parameters in FreeRTOS.
#define configUSE_16_BIT_TICKS 0
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelete 1

// Test manager prototype
void Test_Manager(void);


/*-----------------------------------------------------------*/

int main( void )
 {

    // Test initialisation function.
    test_init();

    //  The first character appears to be ignored by the UUT, so a single character is sent before the
    //  input UART starts to be read.
    UARTSend((unsigned char *)"q", 1, UART1_BASE);

	// Create test management task.
    xTaskCreate(Test_Manager, "MNG", 240, NULL, 1, NULL );

	// Start the scheduler so the tasks start executing.
	vTaskStartScheduler();	
	
	// If all is well we will never reach here as the scheduler will now be
	// running.  If we do reach here then it is likely that there was insufficient
	// heap available for the idle task to be created.
	for( ;; );
}

/*-----------------------------------------------------------*/
//Semaphores are queues declared here as they must be global to work.
xSemaphoreHandle xTestMutex;

// Test manager task, in charge of running and deleting tests.
void Test_Manager()
{
	char cReceived;

	// State list for test manager.
	typedef enum states {IDLE, TEST1, TEST2, TEST3, TEST4} CurrState;
	CurrState state = IDLE;

	xTestMutex = xSemaphoreCreateMutex();

	//Semaphore is taken in order to lower the value from 1 to 0.
	xSemaphoreTake (xPC_SENT, (portTickType)100);

	//Checking mutex exists.
	if (xTestMutex == NULL)
	{
		RIT128x96x4StringDraw("Mutex issue", 5, 20, 30);
	}

	for( ;; )
	{
		if(state == IDLE) //No tests are running.
		{
			if (xCOMMS_FROM_PC_Queue !=0)
			{
				if (xQueueReceive(xCOMMS_FROM_PC_Queue, &cReceived, (portTickType)10))
				{
					if (cReceived == '1') //Command to run test 1.
					{
						test_uart_a_startup();
						state = TEST1;
					} else if (cReceived == '3') //Command to run test 3.
					{
						test_uart_c_startup();
						state = TEST3;
					} else if (cReceived == '4') //Command to run test 4.
					{
						test_uart_d_startup();
						state = TEST4;
					} else
					{
						RIT128x96x4StringDraw("Invalid test number", 5, 20, 30);
					}

					//Timeout is required for all tests, so is always included.

				}

			}
		} else if (state == TEST1) //Test 1 is running.
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				test_uart_a_shutdown();
				state = IDLE;
			}
		} else if (state == TEST3) //Test 3 is running.
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				test_uart_c_shutdown();
				state = IDLE;
			}
		} else if (state == TEST4) //Test 4 is running.
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				test_uart_d_shutdown();
				state = IDLE;
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


