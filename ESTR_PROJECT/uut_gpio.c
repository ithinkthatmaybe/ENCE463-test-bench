/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */



#include "uut_gpio.h"


int g_num_pulses = MAX_NUM_PULSES;

// Globals

// TODO: remove global useage. Have tests initialise own result arrays, use a global pointer to control access
int g_airspeed_pulse_count = -1; // Initialise to -1 so that results are not saved for the first cycle
stopwatch_t g_airspeed_stopwatch;
int g_airspeed_response_flags[MAX_NUM_PULSES] = {0};
unsigned int g_airspeed_times[MAX_NUM_PULSES] = {0};
//
int g_transponder_pulse_count = -1;
stopwatch_t g_transponder_stopwatch;
int g_transponder_response_flags[MAX_NUM_PULSES] = {0};
unsigned int g_transponder_times[MAX_NUM_PULSES] = {0};


// TODO: g_num_pulses is external global, remove usage of this function
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

	//GPIOPinWrite(GPIO_PORTC_BASE, (1<<2), (1<<2)); // debug

	if(g_airspeed_pulse_count >= 0)
		// Proccess last latency measurement
		g_airspeed_times[g_airspeed_pulse_count] = stopwatch_get_time_us(&g_airspeed_stopwatch);

	g_airspeed_pulse_count++;
	stopwatch_start(&g_airspeed_stopwatch);

	if (g_airspeed_pulse_count >= g_num_pulses)
		PWMGenDisable(PWM0_BASE, PWM_GEN_0);

	//GPIOPinWrite(GPIO_PORTC_BASE, (1<<2), ~(1<<2)); // debug
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

/*===================Functions========================*/

void InitGPIO (void)
{
	// GPIO test initialisation
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

	// Configure UUT reset signal
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_4);
	GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_PIN_4);
}



void reset_uut(void){
	 GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, 0);
	 int i = 0;
	 for ( i=0 ; i >100000 ;i++)
	 {continue;}
	 GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_PIN_4);
	 for ( i=0 ; i >100000 ;i++)
	 {continue;}
}

