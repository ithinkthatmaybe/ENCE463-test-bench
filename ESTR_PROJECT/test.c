/*
 * test.c
 *
 *  Created on: Sep 30, 2015
 *      Author: sws52
 */

#include "test.h"

#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)
#define RESULTS_ID_LEN 50


// TODO: will become test one init, or will just setup pins etc
// and task regestration will be split off
void test_init(void)
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

	vSemaphoreCreateBinary( xTEST_DONE );
	xSemaphoreTake (xTEST_DONE, (portTickType)100);

	// UART initialisatoin
	InitUART();
	InitGPIO ();  
	Init_PC_UART();

	// GPIO initialisation
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0 | SYSCTL_PERIPH_PWM);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);	// airspeed output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);	// transponder output
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);	// airspeed response pin
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	// transponder response pin

	// Configure airspeed input
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);

	// Configure transponder input
	GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTB_BASE, &transponder_response_isr);

	// Configure airspeed pulse generation
	GPIOPinTypePWM(GPIO_PORTD_BASE, (1<<1));	// Airspeed output TODO: make pin macro
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMIntDisable(PWM0_BASE, PWM_GEN_0);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD); // configure pwm for end-of-cycle interrupt
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);

	// Configure transponder pulse generation
	GPIOPinTypePWM(GPIO_PORTF_BASE, (1<<3));	// Transponder output TODO: make pin macro
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMIntDisable(PWM0_BASE, PWM_GEN_2);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD); // configure pwm for end-of-cycle interrupt
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_2, transponder_pulse_isr);
}

void test_uart_a_startup(void)
{
	xTaskCreate(vMirrorTX, "MTX", 240, NULL, 5, &xMirrorTX );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
	xTaskCreate(vUART_int_manage, "UIM", 140, NULL, 1, &xUART_int_manage);
	//GPIOPinIntEnable(GPIO_PORTH_BASE, GPIO_PIN_2);
}

void test_uart_a_shutdown(void)
{
	vTaskDelete(xMirrorTX);
	vTaskDelete(xTimeout);
	vTaskDelete(xUART_int_manage);
	vQueueDelete(xToTest);
	vQueueDelete(xToTimeout);

}

void test_uart_ci_startup(void)
{
	xTaskCreate(vStatus, "STS", 240, NULL, 2, &xStatus );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
	xTaskCreate(vUART_int_manage, "UIM", 240, NULL, 1, &xUART_int_manage);


}

void test_uart_ci_shutdown(void)
{
	vTaskDelete(xStatus);
	vTaskDelete(xTimeout);
	vTaskDelete(xUART_int_manage);
	vQueueDelete(xToTest);
	vQueueDelete(xToTimeout);
}

void test_uart_cii_startup(void)
{
	xTaskCreate(vEmergStatus, "STS", 240, NULL, 2, &xEmergStatus );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
	xTaskCreate(vUART_int_manage, "UIM", 240, NULL, 1, &xUART_int_manage);
}

void test_uart_cii_shutdown(void)
{
	vTaskDelete(xEmergStatus);
	vTaskDelete(xTimeout);
	vTaskDelete(xUART_int_manage);
	vQueueDelete(xToTest);
	vQueueDelete(xToTimeout);
}

void test_uart_d_startup(void)
{
	xTaskCreate(vClockSpeed, "CST", 240, NULL, 2, &xClockSpeed );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
}

void test_uart_d_shutdown(void)
{
	vTaskDelete(xClockSpeed);
	vTaskDelete(xTimeout);
	vQueueDelete(xToTest);
	vQueueDelete(xToTimeout);
}


/*-------------------------------TEST TASKS----------------------------*/

// General timeout function for UART tests.
void vTimeout( void )
{
	// Timeout period in ms.
	int timeout = 750;

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
	// The arrays are set to maximum allowed length (plus 1 for \0 character).
	unsigned char mirror[51] = UA_MESSAGE;
	char expect[401] = UA_RESPONSE;

	// Based on length of given strings.
	int len = strlen(mirror);
	int expectedLen = strlen(expect);

	// These must be declared as variables in order to be passed into the queue.
	char pass[5] = "Pass";
	char fail[5] = "Fail";
	char timeout[8] = "Timeout";

	// Variables.
	char  buffer[100]; //= (char *) malloc (expectedLen);
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
	results.test_type = '0';
	results_ptr = &results;

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
		int i = 0;
		int numSent = 1;
		while (done)
		{
			// Loop here while waiting for test manager to delete the task.

			if (i < numSent)
			{
				xSemaphoreTake (xPC_SENT, (portTickType)100);
				i++;
			} else
			{
				xSemaphoreGive(xTEST_DONE);
				while (1)
				{
					vTaskDelay(100);
				}
			}
		}
	}
}

// Status test task.
void vStatus( void )
{
	// This must be declared as a variable in order to be passed into the queue.
	char *status = "s";

	int maxLength = 60; // Chosen as slightly longer than status and transponder message lengths added together.

	// Variables.
	char buffer[60] = {0};
	char cReceived;
	int statusLen = 38;
	int transponderLen = 20;
	int i = 0;

	// Timeout control messages and setup.
	char messageSent = 'S';
	char finished = 'F';
	xToTimeout = xQueueCreate(10, sizeof(char));
	xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

	// Initialising the test results struct.
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '2';
	results_ptr = &results;

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
		// Perform the mirror.
		mirrorUART(status, 1, UART1_BASE, maxLength, buffer);

		while (i < maxLength)
		{
			if (i >= statusLen)
			{
				buffer[i] = '\0';
			} else if (buffer[i] == 'T' && buffer[i+1] == 'P') // Only occurs if a transponder message interrupts the status message.
			{
				statusLen += transponderLen;
			}
			i++;

		}

		// Test passed. Send results to PC and clean up test.
		results.test_string = buffer;
		results.test_string_len = strlen(results.test_string);//maxLength;
		xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
		xQueueSendToBack( xToTimeout, &finished, (portTickType)10);

		int i = 0;
		int numSent = 1;
		while (1)
		{
			// Loop here while waiting for test manager to delete the task.

			if (i < numSent)
			{
				xSemaphoreTake (xPC_SENT, (portTickType)100);
				i++;
			} else
			{
				xSemaphoreGive(xTEST_DONE);
				while (1)
				{
					vTaskDelay(100);
				}
			}
		}
	}
}

void vUART_int_manage(void){

	int time = 0;
	for (;;)
	{
		if (xUART_int_queue !=0)
		{
			while (xQueueReceive(xUART_int_queue, &time, (portTickType)10))
			{
				// Initialising the test results struct.
				Test_res* results_ptr;
				Test_res results = {NULL,NULL,NULL,NULL,NULL};
				results.test_type = 'E';
				results.test_data = &time;
				results.num_of_elements = 1;
				results_ptr = &results;
				xQueueSendFromISR(xSEND_RESULTS_Queue, (void*)&results_ptr, pdFALSE );
			}
		}
	}



}


// Status test task.
void vEmergStatus( void )
{
	// This must be declared as a variable in order to be passed into the queue.
	char *emerg = "e";
	char *reset = "r";

	int maxLength = 60; // Chosen as slightly longer than status and transponder message lengths added together.

	// Variables.
	char buffer[60] = {0};
	char cReceived;

	// Timeout control messages and setup.
	char messageSent = 'S';
	char finished = 'F';
	xToTimeout = xQueueCreate(10, sizeof(char));
	xQueueSendToBack( xToTimeout, &messageSent, (portTickType)10);

	// Initialising the test results struct.
	Test_res* results_ptr;
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	results.test_type = '3';
	results_ptr = &results;

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
		// Perform the mirror.
		mirrorUART(emerg, 1, UART1_BASE, maxLength, buffer);

		// Test passed. Send results to PC and clean up test.
		results.test_string = buffer;
		results.test_string_len = maxLength;
		xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
		xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
		UARTSend(reset, 1, UART1_BASE);
		int i = 0;
		int numSent = 1;
		while (1)
		{
			// Loop here while waiting for test manager to delete the task.

			if (i < numSent)
			{
				xSemaphoreTake (xPC_SENT, (portTickType)100);
				i++;
			} else
			{
				xSemaphoreGive(xTEST_DONE);
				while (1)
				{
					vTaskDelay(100);
				}
			}
		}
	}
}

// Clock speed variation test task.
void vClockSpeed( void )
{
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
	results.test_type = '4';
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
			int i = 0;
			int numSent = 1;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					xSemaphoreTake (xPC_SENT, (portTickType)100);
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
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


//----------------------------GPIO TESTS---------------------------------//

void test_gpio_a_startup(void)
{
	xTaskCreate(vGPIO_a, "gpio_test_a", 240, NULL, 2, &xGPIO_a);
}

void test_gpio_a_shutdown(void)
{
	vTaskDelete(xGPIO_a);

	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 0);

	g_airspeed_pulse_count = -1;
}

void test_gpio_b_startup(void)
{
	xTaskCreate(vGPIO_b, "gpio_test_b", 240, NULL, 2, &xGPIO_b);
}

 void test_gpio_b_shutdown(void)
 {
	vTaskDelete(xGPIO_b);

	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Register standard airspeed pulse generation isr
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 0);

	g_airspeed_pulse_count = -1;
 }

void test_gpio_c_startup(void)
{
	xTaskCreate(vGPIO_c, "gpio_test_c", 240, NULL, 2, &xGPIO_c);
}

void test_gpio_c_shutdown(void)
{
	vTaskDelete(xGPIO_c);

	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0 | PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_5_BIT, 0);

	g_airspeed_pulse_count = -1;
	g_transponder_pulse_count = -1;
}

void test_gpio_d_startup(void)
{
	xTaskCreate(vGPIO_d, "gpio_test_d", 240, NULL, 2, &xGPIO_d);
}

void test_gpio_d_shutdown(void)
{
	vTaskDelete(xGPIO_d);

	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0 | PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_5_BIT, 0);

	g_airspeed_pulse_count = -1;
	g_transponder_pulse_count = -1;
}

void test_gpio_e_startup(void)
{
	xTaskCreate(vGPIO_e, "gpio_test_e", 600, NULL, 2, &xGPIO_e);
}



void test_gpio_e_shutdown(void)
{
	vTaskDelete(xGPIO_e);

	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

	uut_gpio_set_num_pulses(MAX_NUM_PULSES);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 0);


	g_transponder_pulse_count = -1;
}




void vGPIO_a(void)
{
	int iTaskDelayPeriod = 1 / portTICK_RATE_MS;

	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Configure airspeed PWM
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US/2); // 50% duty

	// Enable PWM output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);


	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";

	// Frequency modulation variables

	static char direction = 0;
	static int period = TEST_ONE_MAX_PERIOD_US;
	for(;;)
	{
		// Modulate pulse frequency
		if (direction == 0)
		{
			period-= TEST_ONE_FREQ_STEP_US;
			if (period < TEST_ONE_MIN_PERIOD_US)
				direction = 1;
		}
		else
		{
			period+= TEST_ONE_FREQ_STEP_US;
			if (period > TEST_ONE_MAX_PERIOD_US)
				direction = 0;
		}
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period*CYCLES_PER_US);
		PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, (period*CYCLES_PER_US)/2);


		if (g_airspeed_pulse_count >= MAX_NUM_PULSES) // Test finished
		{
			airspeed_response_results.test_type = '5';
			airspeed_response_results.test_data = (void *) &g_airspeed_response_flags;
			airspeed_response_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_response_results.test_string = (void *) &airspeed_response_id;
			airspeed_response_results.test_string_len = strlen(airspeed_response_id);

			airspeed_latency_results.test_type = '5';
			airspeed_latency_results.test_data = (void *) &g_airspeed_times;
			airspeed_latency_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_latency_results.test_string = (void *) &airspeed_latency_id;
			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);

			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);

			int i = 0;
			int numSent = 2;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					while (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdFALSE)
					{
						continue;
					}
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
			}

		}
		vTaskDelay(iTaskDelayPeriod);
	}
}

void vGPIO_b (void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;

	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_TWO_INIT_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_TWO_MIN_PERIOD_US*CYCLES_PER_US/2);

	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr_gpio_test_b);

	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);

	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";


	//static int period = TEST_TWO_INIT_PERIOD_US;

	for (;;)
	{

//		if (period > TEST_TWO_MIN_PERIOD_US + TEST_TWO_FREQ_STEP_US)
//		{
//			period -= TEST_TWO_FREQ_STEP_US;
//			PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period*CYCLES_PER_US);
//		}

		// End of test
		if (g_airspeed_pulse_count >= MAX_NUM_PULSES)
		{
			airspeed_response_results.test_type = '6';
			airspeed_response_results.test_data = (void *) &g_airspeed_response_flags;
			airspeed_response_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_response_results.test_string =(void *) &airspeed_response_id;
			airspeed_response_results.test_string_len = strlen(airspeed_response_id);

			airspeed_latency_results.test_type = '6';
			airspeed_latency_results.test_data = (void *) &g_airspeed_times;
			airspeed_latency_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_latency_results.test_string = (void *) &airspeed_latency_id;
			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);

			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);

			int i = 0;
			int numSent = 2;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					while (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdFALSE)
					{
						continue;
					}
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
			}


		}
		vTaskDelay(iTaskDelayPeriod);
	}
}


void vGPIO_c (void)
{
	// Enable response interrupts
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);


	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_THREE_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_THREE_PERIOD_US*CYCLES_PER_US/2);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_THREE_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_THREE_PERIOD_US*CYCLES_PER_US/2);

	// Enable pwm output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);

	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);

	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";

	// Initialise result structures
	Test_res transponder_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* transponder_response_results_ptr = &transponder_response_results;
	char transponder_response_id[RESULTS_ID_LEN] = "transponder_response_flags\0";

	Test_res transponder_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* transponder_latency_results_ptr = &transponder_latency_results;
	char transponder_latency_id[RESULTS_ID_LEN] = "transponder_latency\0";

	for (;;)
	{
		if (g_airspeed_pulse_count >= g_num_pulses && g_transponder_pulse_count >= g_num_pulses)
		{
			airspeed_response_results.test_type = '7';
			airspeed_response_results.test_data = (void *) &g_airspeed_response_flags;
			airspeed_response_results.num_of_elements = g_num_pulses;
			airspeed_response_results.test_string =(void *) &airspeed_response_id;
			airspeed_response_results.test_string_len = strlen(airspeed_response_id);

			airspeed_latency_results.test_type = '7';
			airspeed_latency_results.test_data = (void *) &g_airspeed_times;
			airspeed_latency_results.num_of_elements = g_num_pulses;
			airspeed_latency_results.test_string = (void *) &airspeed_latency_id;
			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);

			transponder_response_results.test_type = '7';
			transponder_response_results.test_data = (void *) &g_transponder_response_flags;
			transponder_response_results.num_of_elements = g_num_pulses;
			transponder_response_results.test_string =(void *) &transponder_response_id;
			transponder_response_results.test_string_len = strlen(transponder_response_id);

			transponder_latency_results.test_type = '7';
			transponder_latency_results.test_data = (void *) &g_transponder_times;
			transponder_latency_results.num_of_elements = g_num_pulses;
			transponder_latency_results.test_string = (void *) &transponder_latency_id;
			transponder_latency_results.test_string_len = strlen(transponder_latency_id);


			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_latency_results_ptr, (portTickType)10);

			int i = 0;
			int numSent = 4;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					while (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdFALSE)
					{
						continue;
					}
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
			}
		}
	}
}


void vGPIO_d (void)
{
	// Enable response interrupts
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Configure airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_FOUR_PERIOD_US_A*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_FOUR_PERIOD_US_A*CYCLES_PER_US/2);

	// Configure transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FOUR_PERIOD_US_B*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FOUR_PERIOD_US_B*CYCLES_PER_US/2);

	// Enable pwm output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);

	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";

	// Initialise result structures
	Test_res transponder_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* transponder_response_results_ptr = &transponder_response_results;
	char transponder_response_id[RESULTS_ID_LEN] = "transponder_response_flags\0";

	Test_res transponder_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* transponder_latency_results_ptr = &transponder_latency_results;
	char transponder_latency_id[RESULTS_ID_LEN] = "transponder_latency\0";

	for (;;)
	{
		if (g_airspeed_pulse_count >= g_num_pulses && g_transponder_pulse_count >= g_num_pulses)
		{
			airspeed_response_results.test_type = '8';
			airspeed_response_results.test_data = (void *) &g_airspeed_response_flags;
			airspeed_response_results.num_of_elements = g_num_pulses;
			airspeed_response_results.test_string =(void *) &airspeed_response_id;
			airspeed_response_results.test_string_len = strlen(airspeed_response_id);

			airspeed_latency_results.test_type = '8';
			airspeed_latency_results.test_data = (void *) &g_airspeed_times;
			airspeed_latency_results.num_of_elements = g_num_pulses;
			airspeed_latency_results.test_string = (void *) &airspeed_latency_id;
			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);

			transponder_response_results.test_type = '8';
			transponder_response_results.test_data = (void *) &g_transponder_response_flags;
			transponder_response_results.num_of_elements = g_num_pulses;
			transponder_response_results.test_string =(void *) &transponder_response_id;
			transponder_response_results.test_string_len = strlen(transponder_response_id);

			transponder_latency_results.test_type = '8';
			transponder_latency_results.test_data = (void *) &g_transponder_times;
			transponder_latency_results.num_of_elements = g_num_pulses;
			transponder_latency_results.test_string = (void *) &transponder_latency_id;
			transponder_latency_results.test_string_len = strlen(transponder_latency_id);


			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_latency_results_ptr, (portTickType)10);

			int i = 0;
			int numSent = 4;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					while (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdFALSE)
					{
						continue;
					}
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
			}
		}
	}
}

void vGPIO_e(void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;
	//Register Transponder response ISR
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	SysCtlPWMClockSet(SYSCTL_PWMDIV_8);

	uut_gpio_set_num_pulses(TEST_FIVE_NUM_PULSES);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FIVE_PERIOD_MS*CYCLES_PER_MS/8); // TODO: somethings wrong here
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FIVE_PERIOD_MS*CYCLES_PER_MS/8/2);

	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);


	// Initialise result structures
	Test_res transponder_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* transponder_response_results_ptr = &transponder_response_results;
	char transponder_response_id[RESULTS_ID_LEN] = "transponder_response_flags\0";

	Test_res transponder_message_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* transponder_message_results_ptr = &transponder_message_results;

	char buffer[300];
	char buff;
	int i = 0;
	for (;;)
	{
		while(xQueueReceive(xUARTReadQueue, &buff, (portTickType)10))
		{
			buffer[i++] = buff;
		}

		if (g_transponder_pulse_count >= g_num_pulses)
		{
			vTaskDelay(500 / portTICK_RATE_MS); // Wait for last uart characters to be recieved

			transponder_response_results.test_type = '9';
			transponder_response_results.test_data = (void *) &g_transponder_response_flags;
			transponder_response_results.num_of_elements = g_num_pulses;
			transponder_response_results.test_string =(void *) &transponder_response_id;
			transponder_response_results.test_string_len = strlen(transponder_response_id);

			transponder_message_results.test_type = '9';
			transponder_message_results.test_string = (void *) &buffer;
			transponder_message_results.test_string_len = strlen(buffer);

			// TODO: send transponder DATA also

			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_message_results_ptr, (portTickType)10);


			int i = 0;
			int numSent = 2;
			while (1)
			{
				// Loop here while waiting for test manager to delete the task.

				if (i < numSent)
				{
					while (xSemaphoreTake (xPC_SENT, (portTickType)100) == pdFALSE)
					{
						continue;
					}
					i++;
				} else
				{
					xSemaphoreGive(xTEST_DONE);
					while (1)
					{
						vTaskDelay(100);
					}
				}
			}
		}
		vTaskDelay(iTaskDelayPeriod);
	}
}

