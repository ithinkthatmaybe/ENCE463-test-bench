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

void Test_Manager(void);

void vTimeout( void );
xTaskHandle xTimeout;
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
xQueueHandle xToTest;
xQueueHandle xToTimeout;


// Test manager task, in charge of running and deleting tests.
void Test_Manager()
{
	char cReceived;

	// State list for test manager.
	typedef enum states {IDLE, TEST1, TEST2} CurrState;
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
						xTaskCreate(vMirrorTX, "MTX", 240, NULL, 5, &xMirrorTX );
						state = TEST1;
					} else if (cReceived == '2') //Command to run test 2.
					{
						xTaskCreate(vClockSpeed, "CST", 240, NULL, 2, &xClockSpeed );
						state = TEST2;
					} else
					{
						RIT128x96x4StringDraw("Invalid test number", 5, 20, 30);
					}

					//Timeout is required for all tests, so is always included.
					xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
				}

			}
		} else if (state == TEST1) //Test 1 is running.
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				//Delete tasks that are not required.
				vTaskDelete(xMirrorTX);
				vTaskDelete(xTimeout);
				state = IDLE;
			}
		} else if (state == TEST2) //Test 2 is running.
		{
			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				//Delete tasks that are not required.
				vTaskDelete(xClockSpeed);
				vTaskDelete(xTimeout);
				state = IDLE;
			}
		}

	}
}

/*-----------------------------------------------------------*/



void vTimeout( void )
{
	// Timeout period in ms.
	int timeout = 200;

	// Create queue to send timeout messages to the test.
	xToTest = xQueueCreate(1, sizeof(char));

	stopwatch_t stopwatch;
	unsigned long time = 0;
	char cReceived;
	int finished = 0;

	// Start stopwatch in stopped mode.
	int stopped = 1;

	// A bar is used as it should not be used in normal UART communication.
	char fail = '|';

	for( ;; )
	{
		if (xToTimeout !=0 && stopped == 1) // Timeout in idle mode, waiting for signal to begin.
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				// Receive command to start the timer.
				if (cReceived == 'S')
				{
					stopped = 0;
					stopwatch_start(&stopwatch);
				} else
				{
					RIT128x96x4StringDraw("ERROR STOPWATCH", 5, 20, 30);
				}
			}
		} else if (stopped == 0)
		{
			// Update current time.
			stopwatch_stop(&stopwatch);
			time = stopwatch_get_time_ms(&stopwatch);
		}

		// If current time is greater than timeout period, let the testing task know that it has timed out.
		if (time >=timeout && !finished)
		{
			xQueueSendToBack( xUARTReadQueue, &fail, (portTickType)10);
		}

		// Check the queue for messages to stop the timeout stopwatch.
		if (xToTimeout !=0 && stopped == 0)
		{
			if (xQueueReceive(xToTimeout, &cReceived, (portTickType)10))
			{
				if (cReceived == 'F') // All timing finished for this test.
				{
					finished = 1;
				} else if (cReceived == 'E') // Timing paused, but will resume.
				{
					stopped = 1;
					time = 0;
				}
			}
		}
	}
}

// Mirror TX test task.
void vMirrorTX( void )
{
	// TODO: SET THESE UP TO BE RECEIVED FROM PC
	unsigned char mirror[50] = "`123`456";
	char expect[50] = "ECHO ON123ECHO OFFNACKNACKNACK";

	// Based on length of given strings.
	int len = strlen(mirror);
	int expectedLen = strlen(expect);

	// These must be declared as variables in order to be passed into the queue.
	char pass[5] = "Pass";
	char fail[5] = "Fail";
	char timeout[8] = "Timeout";

	// Variables.
	char * buffer = (char *) malloc (expectedLen);
	int done = 0;
	char cReceived;

	// Timeout control messages and setup.
	char messageSent = 'S';
	char finished = 'F';
	xToTimeout = xQueueCreate(10, sizeof(char));
	xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

	// Initialising the test results struct.
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '1';
	results_ptr = &results;

	// Clears the queue due to an issue where an 'l' was in the queue whne initialized.
	if (xUARTReadQueue !=0)
	{
		while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
		{
			cReceived = 0;
		}
	}

	for( ;; )
	{
		// Perform the mirror.
		mirrorUART(mirror, len, UART1_BASE, expectedLen, buffer);

		// Check the mirror results.
		if (strncmp(buffer, expect, expectedLen)==0)
		{
			// Test passed. Send results to PC and clean up test.
			results.test_string = pass;
			results.test_string_len = strlen(results.test_string);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
			RIT128x96x4StringDraw("Expected response", 5, 10, 30);
			xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
			free(buffer);
			done = 1;
		} else
		{
			// Test failed. Send results to PC and clean up test.
			results.test_string = fail;
			results.test_string_len = strlen(results.test_string);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
			RIT128x96x4StringDraw("Fail wrong received", 5, 10, 30);
			xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
			free(buffer);
			done = 1;
		}

		// Check for timeout.
		if (xToTest !=0 && done == 0)
		{
			while (xQueueReceive(xToTest, &cReceived, (portTickType)10))
			{
				if (cReceived == '|')
				{
					// Test timed out. Send results to PC and clean up test.
					results.test_string = timeout;
					results.test_string_len = strlen(results.test_string);
					xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
					RIT128x96x4StringDraw("Fail timeout", 5, 20, 30);
					xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
					free(buffer);
					done = 1;
				}
			}
		}

		while (done)
		{
			// Loop here while waiting for test manager to delete the task.
			vTaskDelay(100);
		}
	}
}

// Clock speed variation test task.
void vClockSpeed( void )
{
	// TODO: SET THESE UP TO BE RECEIVED FROM PC
	unsigned char mirrorDec[10] = "-`123`";
	unsigned char mirrorInc[10] = "+`123`";
	char expect[20] = "ECHO ON123ECHO OFF";

	// Based on length of given strings.
	int len = strlen(mirrorDec);
	int expectedLen = strlen(expect);

	// Variables
	int changes = 0;
	int increases = 0;
	int decreases = 0;
	char strbuff[20] = {0};
	char * buffer = (char *) malloc (expectedLen);
	int i = 0;
	char cReceived;

	typedef enum states {DEC, INC, FIN} CurrState;
	CurrState state = DEC;

	// Timeout messages and queue.
	char messageSent = 'S';
	char ended = 'E';
	xToTimeout = xQueueCreate(10, sizeof(char));

	// Initialising results struct.
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '2';
	results_ptr = &results;

	// Reset the UUT and wait for it to boot up.
	reset_uut();
	vTaskDelay(500);

	// Clears the queue before running the test to ensure predictable operation.
	if (xUARTReadQueue !=0)
	{
		while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
		{
			cReceived = 0;
		}
	}

	for( ;; )
	{
		// Start the timeout stopwatch.
		xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

		// Perform the mirror test.
		if (state == DEC)
		{
			mirrorUART(mirrorDec, len, UART1_BASE, expectedLen, buffer);
		} else if (state == INC)
		{
			mirrorUART(mirrorInc, len, UART1_BASE, expectedLen, buffer);
		}

		// Check results.
		if (strncmp(buffer, expect, expectedLen)==0) // Received expected response.
		{
			// Stop the timeout stopwatch.
			xQueueSendToBack( xToTimeout, &ended, (portTickType)10);
		} else if (state == DEC) // Received incorrect response (or timed out) while attempting to decrease clock speed.
		{
			// Stop the timeout stopwatch.
			xQueueSendToBack( xToTimeout, &ended, (portTickType)10);
			reset_uut();
			vTaskDelay(100);
			decreases = changes;

			// Changes is set to -1 as this will increase once before any testing occurs.
			changes = -1;

			state = INC;
		} else if (state == INC)// Received incorrect response (or timed out) while attempting to increase clock speed.
		{
			// Test complete. Send results to PC and clean up test.
			increases = changes;
			sprintf(strbuff,"Inc: %d Dec: %d", increases, decreases);
			int data [2]= {0};
			data[0] = increases;
			data[1] = decreases;
			results.test_data = data;
			results.num_of_elements = 2;
			reset_uut();
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			RIT128x96x4StringDraw(strbuff, 5, 20, 30);

			state = FIN;
		} else if (state == FIN)
		{
			free(buffer);
			while (1)
			{
				// When finished, wait here for task to be deleted.
				vTaskDelay(100);
			}
		}

		// Clear results buffer between trials.
		while (i < expectedLen)
		{
			buffer[i] = 0;
			i++;
		}
		i = 0;
		changes++;
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


