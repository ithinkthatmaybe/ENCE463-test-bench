/*
 * test.c
 *
 *  Created on: Sep 30, 2015
 *      Author: sws52
 */

#include "test.h"

#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)
#define RESULTS_ID_LEN 50



void test_init(void)
{
	// Set the clocking to run from the PLL at 50 MHz.  Assumes 8MHz XTAL,
	// whereas some older eval boards used 6MHz.
	SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
	IntMasterEnable();
	// Initialize the OLED display and write status.

	vSemaphoreCreateBinary( xTEST_DONE );
	xSemaphoreTake (xTEST_DONE, (portTickType)100);

	// UART initialisatoin
	InitUART();
	InitGPIO ();  
	Init_PC_UART();


}

/*---------------------UART TEST CONTROL FUNCTIONS----------------*/

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


/*---------------------UART TEST TASKS--------------------------*/

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
	UARTClearReadBuffer();
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
			xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
			free(buffer);
			done = 1;
		} else
		{
			// Test failed. Send results to PC and clean up test.
			results.test_string = fail;
			results.test_string_len = strlen(results.test_string);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
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
					xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
					free(buffer);
					done = 1;
				}
			}
		}
		vTest_wait_for_send(1);
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
	UARTClearReadBuffer();

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

		vTest_wait_for_send(1);
	}
}

void vUART_int_manage(void)
{

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
	UARTClearReadBuffer();

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

		vTest_wait_for_send(1);
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
	int data [2]= {0};

	typedef enum states {DEC, INC, FIN} CurrState;
	CurrState state = DEC;

	// Timeout messages and queue.
	char messageSent = 'S';
	char ended = 'E';
	xToTimeout = xQueueCreate(10, sizeof(char));

	// Initialising results struct.
	Test_res results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* results_ptr;
	results.test_type = '4';
	results_ptr = &results;

	// Reset the UUT and wait for it to boot up.
	reset_uut();
	vTaskDelay(500);


	// Clears the queue before running the test to ensure predictable operation.
	UARTClearReadBuffer();

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

			data[0] = increases;
			data[1] = decreases;
			results.test_data = data;
			results.num_of_elements = 2;
			reset_uut();
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);

			state = FIN;
		} else if (state == FIN)
		{
			free(buffer);
			vTest_wait_for_send(1); // Wait for results to finish sending
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


/*---------------------GPIO TEST CONTROL FUNCTIONS----------------*/

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

	// Reset result arrays
	int i;
	for (i = 0; i < MAX_NUM_PULSES; i++)
	{
		g_airspeed_response_flags[i] = 0;
	}

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

	// Reset result arrays
	int i;
	for (i = 0; i < MAX_NUM_PULSES; i++)
	{
		g_airspeed_response_flags[i] = 0;
	}
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


	// Reset result arrays
	int i;
	for (i = 0; i < MAX_NUM_PULSES; i++)
	{
		g_airspeed_response_flags[i] = 0;
		g_transponder_response_flags[i] = 0;
	}
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

	// Reset result arrays
	int i;
	for (i = 0; i < MAX_NUM_PULSES; i++)
	{
		g_airspeed_response_flags[i] = 0;
		g_transponder_response_flags[i] = 0;
	}
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

	GPIOPinTypePWM(GPIO_PORTF_BASE, (1<<3));

	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

	uut_gpio_set_num_pulses(MAX_NUM_PULSES);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 0);

	// Reset result arrays
	int i;
	for (i = 0; i < MAX_NUM_PULSES; i++)
	{
		g_transponder_response_flags[i] = 0;
	}
	g_transponder_pulse_count = -1;
}


/*----------------------GPIO TEST TASKS--------------------------*/

// Variable frequency, variable width pulse train (rising edges on P2.5 / pin 13, see Table 5) simulating
// Airspeed propeller pulses; provide checks for latency and 1:1 response.
void vGPIO_a(void)
{
	int iTaskDelayPeriod = 1 / portTICK_RATE_MS;

	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";

		// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Configure airspeed PWM
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, GPIO_A_MAX_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, GPIO_A_MAX_PERIOD_US*CYCLES_PER_US/2); // 50% duty

	// Enable PWM output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);

	// Frequency modulation variables
	static char direction = 0;
	static int period = GPIO_A_MAX_PERIOD_US;

	for(;;)
	{
		// Modulate pulse frequency
		if (direction == 0)
		{
			period-= GPIO_A_FREQ_STEP_US;
			if (period < GPIO_A_MIN_PERIOD_US)
				direction = 1;
		}
		else
		{
			period+= GPIO_A_FREQ_STEP_US;
			if (period > GPIO_A_MAX_PERIOD_US)
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

			vTest_wait_for_send(2);

		}
		vTaskDelay(iTaskDelayPeriod);
	}
}

// Randomly occurring 2- or 3-pulse bursts down to some minimum specified pulse-to-pulse interval
// (rising edges on P2.5 / pin 13) simulating Airspeed propeller pulses; provide checks for latency and 1:1
// response
void vGPIO_b (void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;

	// Initialise result structures
	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";

	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";

	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, GPIO_B_INIT_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, GPIO_B_MIN_PERIOD_US*CYCLES_PER_US/2);

	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);

	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
	static int period = GPIO_B_INIT_PERIOD_US;

	for (;;)
	{

		if (period > GPIO_B_MIN_PERIOD_US + GPIO_B_FREQ_STEP_US)
		{
			period -= GPIO_B_FREQ_STEP_US;
			PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period*CYCLES_PER_US);
		}

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

			vTest_wait_for_send(2);
		}
		vTaskDelay(iTaskDelayPeriod);
	}
}


// Generate synchronous events on two pins (synchronous rising edges on both P2.5 / pin 13 and P1.7 /
// pin 15) simulating Airspeed propeller and Tracking radar pulses; check for interference.
void vGPIO_c (void)
{


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


	// Enable response interrupts
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);


	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, GPIO_C_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, GPIO_C_PERIOD_US*CYCLES_PER_US/2);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, GPIO_C_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, GPIO_C_PERIOD_US*CYCLES_PER_US/2);

	// Enable pwm output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);

	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);

	// Wait for test to finish
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

			vTest_wait_for_send(4);
		}
	}
}


// Generate asynchronous events on two pins; provide statistics on latency and 1:1 response.
void vGPIO_d (void)
{


	// Initialise result structures	

	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";
	Test_res airspeed_response_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_response_results_ptr = &airspeed_response_results;


	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";
	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;

	char transponder_response_id[RESULTS_ID_LEN] = "transponder_response_flags\0";
	Test_res transponder_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* transponder_response_results_ptr = &transponder_response_results;

	char transponder_latency_id[RESULTS_ID_LEN] = "transponder_latency\0";
	Test_res transponder_latency_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* transponder_latency_results_ptr = &transponder_latency_results;

	for (;;)
	{
		// Check for completion of test
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

			vTest_wait_for_send(4);
		}
	}
}


// Invoke a transponder message (rising edge on P1.7 / pin 15) simulating the Tracking radar pulses; check
// reception of message.
void vGPIO_e(void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;
	//Register Transponder response ISR

	// Initialise result structures
	Test_res transponder_response_results = {NULL,NULL,NULL,NULL,NULL};
	Test_res* transponder_response_results_ptr = &transponder_response_results;
	char transponder_response_id[RESULTS_ID_LEN] = "transponder_response_flags\0";

	Test_res transponder_message_results = {NULL, NULL, NULL, NULL, NULL};
	Test_res* transponder_message_results_ptr = &transponder_message_results;

	// Variables.
	int maxLength = 60;
	char buffer[60] = {0};
	char cReceived;
	int transponderLen = 20;
	int i = 0;

	// Clears the queue before running the test to ensure predictable operation.
	UARTClearReadBuffer();

		// Generate a single pulse on the transponder pulse output,
		// because we're only testing one pulse at this stage

	//TODO: test for several pulses
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, (1<<3));

	g_transponder_pulse_count = 0;
	GPIOPinWrite(GPIO_PORTF_BASE, (1<<3), (1<<3));
	vTaskDelay(1);
	GPIOPinWrite(GPIO_PORTF_BASE, (1<<3), ~(1<<3));

	vTaskDelay(500 / portTICK_RATE_MS); //Wait to let the uut respond

	for (;;)
	{
		// Read response
		while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
		{
			buffer[i++] = cReceived;
		}

		i = 0;

		// Append termination caracter
		while (i < maxLength)
		{
			if (i >= transponderLen)
			{
				buffer[i] = '\0';
			}
			i++;
		}

		// Transmit back to PC
		vTaskDelay(500 / portTICK_RATE_MS); // Wait for last uart characters to be recieved

		transponder_response_results.test_type = '9';
		transponder_response_results.test_data = (void *) &g_transponder_response_flags;
		transponder_response_results.num_of_elements = 1;
		transponder_response_results.test_string =(void *) &transponder_response_id;
		transponder_response_results.test_string_len = strlen(transponder_response_id);

		transponder_message_results.test_type = '9';
		transponder_message_results.test_string = (void *) &buffer;
		transponder_message_results.test_string_len = strlen(buffer);

		// TODO: send transponder DATA also

		xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_response_results_ptr, (portTickType)10);
		xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&transponder_message_results_ptr, (portTickType)10);

		vTest_wait_for_send(2);	// TODO: macro: TEST_E_NUM_PACKETS
	}
}

// Infinate loop untill PC uart finishes sending all results back to python
// Then give a semaphore back to the test manager and wait to be killed
void vTest_wait_for_send(int numSent)
{
	int i = 0;
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
