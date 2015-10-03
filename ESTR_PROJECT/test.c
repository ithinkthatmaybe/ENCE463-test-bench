/*
 * test.c
 *
 *  Created on: Sep 30, 2015
 *      Author: sws52
 */

#include "test.h"

#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)


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
	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);

	// Configure airspeed pulse generation
	GPIOPinTypePWM(GPIO_PORTD_BASE, (1<<1));	// Airspeed output TODO: make pin macro
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMIntDisable(PWM0_BASE, PWM_GEN_0);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD); // configure pwm for end-of-cycle interrupt
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);

	// Configure transponder pulse generation
	GPIOPinTypePWM(GPIO_PORTF_BASE, (1<<3));	// Transponder output TODO: make pin macro
}

void test_uart_a_startup(void)
{
	xTaskCreate(vMirrorTX, "MTX", 240, NULL, 5, &xMirrorTX );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
}

void test_uart_a_shutdown(void)
{
	vTaskDelete(xMirrorTX);
	vTaskDelete(xTimeout);
}

void test_uart_c_startup(void)
{
	xTaskCreate(vStatus, "STS", 240, NULL, 2, &xStatus );
	xTaskCreate(vTimeout, "TIME", 240, NULL, 2, &xTimeout );
}

void test_uart_c_shutdown(void)
{
	vTaskDelete(xStatus);
	vTaskDelete(xTimeout);
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
}

void test_gpio_a_startup(void)
{
	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Configure airspeed PWM
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD*CYCLES_PER_US/2); // 50% duty

	// Enable PWM output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);

	// Register background tasks
//	xTaskCreate( test_one_frequency_mod, "test one Frequency variation", 1000, NULL, 1, NULL);
}

void test_gpio_a_shutdown(void)
{
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 0);
}

void test_gpio_b_startup(void)
{
	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD*CYCLES_PER_US/2);

	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr_gpio_test_b);

	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);

	//register background tasks
//	xTaskCreate( test_two_frequency_mod, "test two Frequency variation", 1000, NULL, 1, NULL);
}

 void test_gpio_b_shutdown(void)
 {
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Register standard airspeed pulse generation isr
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 0);
 }

void test_gpio_c_startup(void)
{
	// Enable response interrupts
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);


	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_THREE_PERIOD*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_THREE_PERIOD*CYCLES_PER_US/2);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_THREE_PERIOD*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_THREE_PERIOD*CYCLES_PER_US/2);

	// Enable pwm output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0 | PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_5_BIT, 1);
}

void test_gpio_c_shutdown(void)
{
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0 | PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_5_BIT, 0);
}

void test_gpio_d_startup(void)
{
	// Enable response interrupts
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Configure airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_FOUR_PERIOD_A*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_FOUR_PERIOD_A*CYCLES_PER_US/2);

	// Configure transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FOUR_PERIOD_B*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FOUR_PERIOD_B*CYCLES_PER_US/2);

	// Enable pwm output
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);
}

void test_gpio_d_shutdown(void)
{
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_0 | PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_5_BIT, 0);
}

void test_gpio_e_startup(void)
{
	//Register Transponder response ISR
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FOUR_PERIOD_B*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FOUR_PERIOD_B*CYCLES_PER_US/2);

	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);
}

void test_gpio_e_shutdown(void)
{
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 0);
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

		while (done)
		{
			// Loop here while waiting for test manager to delete the task.
			vTaskDelay(100);
		}
	}
}

// Status test task.
void vStatus( void )
{
	// These must be declared as variables in order to be passed into the queue.
	char pass[5] = "Pass";
	char fail[5] = "Fail";
	char timeout[8] = "Timeout";
	char *status = "s";
	char *emerg = "e";
	char *resetEmerg = "r";

	int maxLength = 60; // Chosen as slightly longer than status and transponder message lengths added together.

	// Variables.
	char buffer[60] = {0};
	typedef enum states {STATUS, EMERG, RESET, FINISH} CurrState;
	CurrState state = STATUS;
	char cReceived;
	int done = 0;

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
		mirrorUART(status, 1, UART1_BASE, maxLength, buffer);

		// Test passed. Send results to PC and clean up test.
		results.test_string = buffer;
		results.test_string_len = maxLength;
		xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&results_ptr, (portTickType)10);
		xQueueSendToBack( xToTimeout, &finished, (portTickType)10);
		done = 1;
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




