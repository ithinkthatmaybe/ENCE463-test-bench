/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */

#include "include/uut_gpio.h"


// ISR responds to airspeed pulses
void vISRgpio_port_e(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	stopwatch_stop(&g_airspeed_stopwatch);	// TODO: protect stopwatch with mutex
	g_airspeed_response_flag = 1;
	return;
}

// ISR responds to transponder & kernel interrupts
void vISRgpio_port_c(void)
{
	return;
}

void uut_gpio_init(void)
{
	// GPIO init / signs of life
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, STATUS_LED_PG2);
	GPIOPinWrite(GPIO_PORTG_BASE, STATUS_LED_PG2, STATUS_LED_PG2);

	// Setup Airspeed response ISR
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &vISRgpio_port_e);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);


	// Setup Airspeed output
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2);
}

//TODO: Variable width, find minimal detection width
//TODO: Variable frequency, finid maximum operating frequency, or what percentage of total pulses are missed as a function of frequency
//TODO: measure average, maximum response time

// Section requires the testing of airspeed measurement simulation, ie quadrature pulses recieved from an encoder attached to an anemometer
// here we provide variable frequency, variable width pulses by holding a constant duty cycle of 50% and increasing the frequency
void vTaskStimulateAirspeed(void *pvParameters)
{
	int period_width = TEST_ONE_INIT_PERIOD;

	int cycles = 0;
	int timeouts = 0;
	int misses = 0;

	for (;;)
	{

		g_airspeed_response_flag = 0;
		stopwatch_start(&g_airspeed_stopwatch);			// TODO: protect stopwatch with mutex
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, UUT_AIRSPEED_OUTPUT_PIN_PE2);

		// block wait for pulse width
		unsigned int i;
		for (i = 0; i < period_width/2; i++)
			continue;

		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);

		for(i =  0; i < period_width/2; i++)
			continue;

		period_width--;
		unsigned long time = stopwatch_get_time_us(&g_airspeed_stopwatch);




		if (g_airspeed_response_flag != 1)
			misses++;


		if (time >= TEST_ONE_MAX_RESPONSE_TIME_US)
			timeouts++;


		if (period_width <= TEST_ONE_MIN_PERIOD)
		{
			cycles++;
			period_width = TEST_ONE_INIT_PERIOD;
		}


		if (cycles > TEST_ONE_NUM_CYCLES)
		{
			RIT128x96x4Clear();
			RIT128x96x4StringDraw("Test finished", 8, 40, 4);
			char buffer[20] = {0};
			sprintf(buffer, "%d misses", (int)misses);
			RIT128x96x4StringDraw(buffer, 8, 50, 4);
			sprintf(buffer, "%d timeouts", (int)timeouts);
			RIT128x96x4StringDraw(buffer, 8, 60, 4);

			while(1);
		}
	}
}

