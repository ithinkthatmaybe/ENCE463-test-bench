/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */

#include "uut_gpio.h"

// Private globals
stopwatch_t g_airspeed_stopwatch;
char g_airspeed_response_flag;

stopwatch_t g_transponder_stopwatch;
char g_transponder_response_flag;

int g_timeouts = 0;
int g_misses = 0;
int g_overs = 0;

int g_trans_timeouts = 0;
int g_trans_misses = 0;
int g_trans_overs = 0;


int g_pulse_count = 0;

void vTaskDisplayResults(void)
{
	for(;;)
	{
		if (g_pulse_count > TEST_ONE_NUM_PULSES)
		{
			RIT128x96x4Clear();
			RIT128x96x4StringDraw("Test finished", 8, 10, 4);
			char buffer[20] = {0};
			sprintf(buffer, "%d missed responses", (int)g_misses);
			RIT128x96x4StringDraw(buffer, 8, 20, 4);
			sprintf(buffer, "%d extra responses", (int)g_overs);
			RIT128x96x4StringDraw(buffer, 8, 30, 4);
			sprintf(buffer, "%d timeouts", (int)g_timeouts);
			RIT128x96x4StringDraw(buffer, 8, 40, 4);

			sprintf(buffer, "%d missed responses", (int)g_trans_misses);
			RIT128x96x4StringDraw(buffer, 8, 60, 4);
			sprintf(buffer, "%d extra responses", (int)g_trans_overs);
			RIT128x96x4StringDraw(buffer, 8, 70, 4);
			sprintf(buffer, "%d timeouts", (int)g_trans_timeouts);
			RIT128x96x4StringDraw(buffer, 8, 80, 4);


			TimerDisable(TIMER0_BASE, TIMER_A);	//todo: create post test task to unregester interrupts etc
			while(1);
		}
	}
}



// TODO: make duty cycle 50%, possibly using another intterupt, or go to pwm
void airspeed_pulse_generation(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	static int state = 0;	//State variable used to make timer generate a square wave, using a second ISR will be a better solution long term

	if (state == 0 && g_pulse_count <= TEST_ONE_NUM_PULSES)
	{
		state = 1;


		g_pulse_count++;

		// --begins scaffolding--
		// TODO: code from origional background task, temporarily in ISR as scaffolding
		if(g_pulse_count != 1)
		{
			unsigned long time = stopwatch_get_time_us(&g_airspeed_stopwatch);

			// make sure the ISR has occured once and only once
			if (g_airspeed_response_flag == 0)
				g_misses++;

			if (g_airspeed_response_flag > 1)
				g_overs++;

			// make sure the ISR occured within a reasonable time period of the stimluls
			if (time >= TEST_ONE_MAX_RESPONSE_TIME_US)
				g_timeouts++;
		}
		// --Ends scaffolding--

		// --begins test addition--
		if(g_pulse_count != 1)
		{
			unsigned long trans_time = stopwatch_get_time_us(&g_transponder_stopwatch);

			// make sure the ISR has occured once and only once
			if (g_transponder_response_flag == 0)
				g_trans_misses++;

			if (g_transponder_response_flag > 1)
				g_trans_overs++;

			// make sure the ISR occured within a reasonable time period of the stimluls
			if (trans_time >= TEST_ONE_MAX_RESPONSE_TIME_US)
				g_trans_timeouts++;
		}

		GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, UUT_TRANSPONDER_OUTPUT_PIN_PE0);
		stopwatch_start(&g_transponder_stopwatch);
		g_transponder_response_flag = 0;
		// --ends test addition--

		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, UUT_AIRSPEED_OUTPUT_PIN_PE2);
		stopwatch_start(&g_airspeed_stopwatch);
		g_airspeed_response_flag = 0;
	//	transponder_pulse_generation();	//todo: TEMPORARY
	}
	else
	{
		state = 0;
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, ~UUT_TRANSPONDER_OUTPUT_PIN_PE0);
	}

}


//void transponder_pulse_generation(void)
//{
//	// --begins scaffolding--
//	// TODO: code from origional background task, temporarily in ISR as scaffolding
//	if(g_pulse_count != 1)
//	{
//		unsigned long time = stopwatch_get_time_us(&g_transponder_stopwatch);
//
//		// make sure the ISR has occured once and only once
//		if (g_airspeed_response_flag == 0)
//			g_trans_misses++;
//
//		if (g_airspeed_response_flag > 1)
//			g_trans_overs++;
//
//		// make sure the ISR occured within a reasonable time period of the stimluls
//		if (time >= TEST_ONE_MAX_RESPONSE_TIME_US)
//			g_trans_timeouts++;
//	}
//	// --Ends scaffolding--
//
//	GPIOPinWrite(GPIO_PORTC_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, UUT_TRANSPONDER_OUTPUT_PIN_PE0);
//	stopwatch_start(&g_transponder_stopwatch);
//	g_transponder_response_flag = 0;
//}

// ISR responds to airspeed pulses
void vISRgpio_port_e(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	stopwatch_stop(&g_airspeed_stopwatch);	// TODO: protect stopwatch with mutex?
	g_airspeed_response_flag++;
	GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);	//TODO: move this line
}

// ISR responds to transponder & kernel interrupts
void vISRgpio_port_c(void)	//TODO: fix names
{
	GPIOPinIntClear(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	stopwatch_stop(&g_transponder_stopwatch);
	g_transponder_response_flag++;
	GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, ~UUT_TRANSPONDER_OUTPUT_PIN_PE0);
}


// TODO: will become test one init, or will just setup pins etc
// and task regestration will be split off
void uut_gpio_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	// Setup Airspeed & transponder outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2);
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0);

	// Setup Airspeed response ISR
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &vISRgpio_port_e);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
																												//!!!!!!!!!!!!!!!!!!!!!!!!
	//Setup Transponder response ISR																			// FIX PORTC_2 -> PORTE_0
	GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTB_BASE, &vISRgpio_port_c);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

//	//Configure timer for foreground event generation	// TODO: test one specific
	TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);
	TimerEnable(TIMER0_BASE, TIMER_A);
	TimerIntRegister(TIMER0_BASE, TIMER_A, airspeed_pulse_generation);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

//	// Register background tasks
	xTaskCreate( vTaskDisplayResults, "Finished", 1000, NULL, 1, NULL);

}

// Section requires the testing of airspeed measurement simulation, ie quadrature pulses recieved from an encoder attached to an anemometer
// here we provide variable frequency, variable width pulses by holding a constant duty cycle of 50% and increasing the frequency
//void vTaskStimulateAirspeed(void *pvParameters)
//{
//	int period_width = TEST_ONE_INIT_PERIOD;
//
//	//loop iterator
//	int cycles = 0;
//
//	int timeouts = 0;
//	int misses = 0;
//	int overs = 0;
//
//	for (;;)
//	{
//
//		g_airspeed_response_flag = 0;
//		stopwatch_start(&g_airspeed_stopwatch);			// TODO: protect stopwatch with mutex?
//		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, UUT_AIRSPEED_OUTPUT_PIN_PE2);
//
//		// block wait for pulse width
//		unsigned int i;
//		for (i = 0; i < period_width/2; i++)
//			continue;
//
//		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);
//
//		for(i =  0; i < period_width/2; i++)
//			continue;
//
//		period_width--;
//
//		unsigned long time = stopwatch_get_time_us(&g_airspeed_stopwatch);
//
//		// make sure the ISR has occured once and only once
//		if (g_airspeed_response_flag == 0)
//			misses++;
//
//		if (g_airspeed_response_flag > 1)
//			overs++;
//
//		// make sure the ISR occured within a reasonable time period of the stimluls
//		if (time >= TEST_ONE_MAX_RESPONSE_TIME_US)
//			timeouts++;
//
//
//		// we increase the frequency to a certain point, once we reach it, reset and go again.
//		if (period_width <= TEST_ONE_MIN_PERIOD)
//		{
//			cycles++;
//			period_width = TEST_ONE_INIT_PERIOD;
//		}
//
//		// TODO: create test completion function to return results and unregister tasks and ISRs
//		// once it's cycled enough to demonstrate reliable operation, stop and return results.
//		if (cycles > TEST_ONE_NUM_CYCLES)
//		{
//			RIT128x96x4Clear();
//			RIT128x96x4StringDraw("Test finished", 8, 40, 4);
//			char buffer[20] = {0};
//			sprintf(buffer, "%d missed responses", (int)misses);
//			RIT128x96x4StringDraw(buffer, 8, 50, 4);
//			sprintf(buffer, "%d extra responses", (int)overs);
//			RIT128x96x4StringDraw(buffer, 8, 60, 4);
//			sprintf(buffer, "%d timeouts", (int)timeouts);
//			RIT128x96x4StringDraw(buffer, 8, 70, 4);
//
//			// fall into infinate loop, it would be better to suspend the task and let
//			// RTOS fall into the idle task
//			while(1);
//		}
//	}
//}

