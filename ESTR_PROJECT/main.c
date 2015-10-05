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

#define TEST0_REQ '0'
#define TEST1_REQ '1'
#define TEST2_REQ '2'
#define TEST3_REQ '3'
#define TEST4_REQ '4'
#define TEST5_REQ '5'
#define TEST6_REQ '6'
#define TEST7_REQ '7'
#define TEST8_REQ '8'
#define TEST9_REQ '9'

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

/*
 * Tests
 * 0 - UART a
 * 1 - UART b
 * 2 - UART c)i
 * 3 - UART c)ii
 * 4 - UART d
 * 5 - GPIO a
 * 6 - GPIO b
 * 7 - GPIO c
 * 8 - GPIO d
 * 9 - GPIO e
 */

// Test manager task, in charge of running and deleting tests.
void Test_Manager()
{
	char cReceived;

	// State list for test manager.
	typedef enum states {IDLE, TEST0, TEST1, TEST2, TEST3, TEST4,
		TEST5, TEST6, TEST7, TEST8, TEST9} CurrState;	//TODO change to match names in spec ie TEST_G_a, TEST_U_a...

	CurrState state = IDLE;

	xTestMutex = xSemaphoreCreateMutex();

	//Semaphore is taken in order to lower the value from 1 to 0.
	xSemaphoreTake (xTEST_DONE, (portTickType)100);

	//Checking mutex exists.
	if (xTestMutex == NULL)
	{
		RIT128x96x4StringDraw("Mutex issue", 5, 20, 30);
	}
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	for( ;; )
	{

		switch(state)
		{
		case IDLE:
			if (xCOMMS_FROM_PC_Queue !=0)
			{
				if (xQueueReceive(xCOMMS_FROM_PC_Queue, &cReceived, (portTickType)10))
				{
					switch(cReceived)
					{
					case TEST0_REQ :
						test_uart_a_startup();
						state = TEST0;
						break;
					case TEST1_REQ :
						// Test not implemented.
						//test_uart_b_startup();
						state = TEST1;
						break;
					case TEST2_REQ :
						test_uart_ci_startup();
						state = TEST2;
						break;
					case TEST3_REQ :
						test_uart_cii_startup();
						state = TEST3;
						break;
					case TEST4_REQ :
						test_uart_d_startup();
						state = TEST4;
						break;
					case TEST5_REQ :
						test_gpio_a_startup();
						state = TEST5;
						break;
					case TEST6_REQ :
						test_gpio_b_startup();
						state = TEST6;
						break;
					case TEST7_REQ :
						test_gpio_c_startup();
						state  = TEST7;
						break;
					case TEST8_REQ:
						test_gpio_d_startup();
						state = TEST8;
						break;
					case TEST9_REQ:
						test_gpio_e_startup();
						state = TEST9;
						break;
					default:
						RIT128x96x4StringDraw("Invalid test number", 5, 20, 30);
						break;
					}
				}

			}
			break;
		case TEST0 :
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_uart_a_shutdown();
				state = IDLE;
			}
			break;
		case TEST1 :
			results.test_type = '2';
			results_ptr = &results;
			results.test_string = "Test does not exist";
			results.test_string_len = strlen(results.test_string);;
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			if (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdTRUE)
			{
				vTaskDelay(100);
				//test_uart_b_shutdown();
				state = IDLE;
			}
		case TEST2 :
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_uart_ci_shutdown();
				state = IDLE;
			}
			break;
		case TEST3:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_uart_cii_shutdown();
				state = IDLE;
			}
			break;
		case TEST4:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_uart_d_shutdown();
				state = IDLE;
			}
			break;
		case TEST5:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_gpio_a_shutdown();
				state = IDLE;
			}
			break;
		case TEST6:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_gpio_b_shutdown();
				state = IDLE;
			}
			break;
		case TEST7:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_gpio_c_shutdown();
				state = IDLE;
			}
			break;
		case TEST8:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_gpio_d_shutdown();
				state = IDLE;
			}
			break;
		case TEST9:
			if (xSemaphoreTake (xTEST_DONE, (portTickType)100) == pdTRUE)
			{
				test_gpio_e_shutdown();
				state = IDLE;
			}
			break;
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


