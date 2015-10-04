/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */



#include "uut_gpio.h"


int g_num_pulses = MAX_NUM_PULSES;

// Globals
int g_airspeed_pulse_count = -1; // Initialise to -1 so that results are not saved for the first cycle
stopwatch_t g_airspeed_stopwatch;
int g_airspeed_response_flags[MAX_NUM_PULSES] = {0};
unsigned int g_airspeed_times[MAX_NUM_PULSES] = {0};
//
int g_transponder_pulse_count = -1;
stopwatch_t g_transponder_stopwatch;
int g_transponder_response_flags[MAX_NUM_PULSES] = {0};
unsigned int g_transponder_times[MAX_NUM_PULSES] = {0};


void uut_gpio_set_num_pulses(int pulses)
{
	g_num_pulses = pulses;
}


/*=========================ISRs=============================*/


// Upon the PWM rising edge, the current system time is measured, a counter is incremented
// and the number of pulses generated to that point is checked.
void airspeed_pulse_isr(void)
{
	PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);

//	GPIOPinWrite(GPIO_PORTC_BASE, (1<<2), (1<<2));

	if(g_airspeed_pulse_count >= 0)
		// Proccess last latency measurement
		g_airspeed_times[g_airspeed_pulse_count] = stopwatch_get_time_us(&g_airspeed_stopwatch);

	g_airspeed_pulse_count++;
	stopwatch_start(&g_airspeed_stopwatch);

	if (g_airspeed_pulse_count >= g_num_pulses)
		PWMGenDisable(PWM0_BASE, PWM_GEN_0);

//	GPIOPinWrite(GPIO_PORTC_BASE, (1<<2), ~(1<<2));
}

/*-----------------------------------------------------------*/

// Upon the PWM rising edge, the current system time is measured, a counter is incremented
// and the number of pulses generated to that point is checked.
void transponder_pulse_isr(void)
{
	PWMGenIntClear(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD);

	if(g_transponder_pulse_count >= 0)
		g_transponder_times[g_transponder_pulse_count] = stopwatch_get_time_us(&g_transponder_stopwatch);	//TODO: consider if the response hasnt occured

	g_transponder_pulse_count++;
	stopwatch_start(&g_transponder_stopwatch);

	if (g_transponder_pulse_count >= g_num_pulses)
		PWMGenDisable(PWM0_BASE, PWM_GEN_2);
}

/*-----------------------------------------------------------*/
// TODO: bug fix. PWM still generates a small pulse on the cycle that it is disabled
// This pulse is still large enough to be reliably registered by the uut, so
// the smaller pulse on the disable cycle can be used as one of the proper pulses.
// The pulse counter just needs to be incremented in such a way that accounts for this.

// Save as the first airspeed pulse gen ISR as above, however
// The PWM module is disabled after 3 pulses are generated
void airspeed_pulse_isr_gpio_test_b(void)
{
	PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);


	if(g_airspeed_pulse_count >= 1)
		// Proccess last latency measurement
		g_airspeed_times[g_airspeed_pulse_count] = stopwatch_get_time_us(&g_airspeed_stopwatch);

	// Increment the pulse count if output is currently enabled and disable output
	// if two pulses have been generated
	g_airspeed_pulse_count += 1 - test_b_output_toggle();
	stopwatch_start(&g_airspeed_stopwatch);

	if (g_airspeed_pulse_count >= MAX_NUM_PULSES)
		PWMGenDisable(PWM0_BASE, PWM_GEN_0);
}

/*-----------------------------------------------------------*/


int test_b_output_toggle(void)
{
	static int count = 0;
	static int state = 0;
	static int num_waits = 1;

	// generates 3 pulses, looks like 2 on a scope
	// but a very narrow pulse is generated on the cycle where
	// PWM is disabled, this is slightly hacky, but
	// the pulse is reliably recieved by the uut.
 	if (count >= 2 && state == 0)
	{
		state = 1;
		count = 0;

		PWMOutputState(PWM0_BASE, PWM_GEN_1_BIT, 0); //Disable PWM output

		return 0; // Because a pulse is still generated this cycle
	}
	else if (count > num_waits + 5 && state == 1)
	{
		num_waits = (num_waits * 283 + 23)%7; // make some pseudorandomness

		state = 0;
		count = 0;

		PWMOutputState(PWM0_BASE, PWM_GEN_1_BIT, 1); // Reenable output

	}

	count++;
	return state;
}

/*-----------------------------------------------------------*/

// ISR responds to airspeed pulses
void airspeed_response_isr(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	stopwatch_stop(&g_airspeed_stopwatch);	// TODO: protect stopwatch with mutex?
	g_airspeed_response_flags[g_airspeed_pulse_count]++;
}

/*-----------------------------------------------------------*/

// ISR responds to transponder interrupts
void transponder_response_isr(void)
{
	GPIOPinIntClear(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	stopwatch_stop(&g_transponder_stopwatch);
	g_transponder_response_flags[g_transponder_pulse_count]++;
}



