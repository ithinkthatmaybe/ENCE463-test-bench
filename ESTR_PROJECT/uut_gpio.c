/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */



#include "uut_gpio.h"



#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)




int g_airspeed_pulse_count = 0;
stopwatch_t g_airspeed_stopwatch;
unsigned int g_airspeed_response_flags[TEST_ONE_NUM_PULSES] = {0};
unsigned int g_airspeed_times[TEST_ONE_NUM_PULSES] = {0};

//int g_airspeed_timeouts = 0;
//int g_airspeed_misses = 0;
//int g_airspeed_overs = 0;



int g_transponder_pulse_count = 0;
stopwatch_t g_transponder_stopwatch;
char g_transponder_response_flags[TEST_ONE_NUM_PULSES] = {0};
unsigned int g_transponder_times[TEST_ONE_NUM_PULSES] = {0};

//int g_transponder_timeouts = 0;
//int g_transponder_misses = 0;
//int g_transponder_over = 0;



// PROTOTYPES

// void uut_gpio_response_analysis(char response_flag, unsigned int time, int *g_out_misses,
// 		int *g_out_overs, int *g_out_timeouts);

// Background tasks
// void vTaskDisplayResults(void);



/*=========================ISRs=============================*/


// Upon the PWM rising edge, the current system time is measured, a counter is incremented
// and the number of pulses generated to that point is checked.
void airspeed_pulse_isr(void)
{
	PWMGenIntClear(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);

	if(g_airspeed_pulse_count >= 1)
		// Proccess last latency measurement
		g_airspeed_times[g_airspeed_pulse_count] = stopwatch_get_time_us(&g_airspeed_stopwatch);

	g_airspeed_pulse_count++;
	stopwatch_start(&g_airspeed_stopwatch);

	if (g_airspeed_pulse_count >= TEST_ONE_NUM_PULSES)
		PWMGenDisable(PWM0_BASE, PWM_GEN_0);
}

/*-----------------------------------------------------------*/

// Upon the PWM rising edge, the current system time is measured, a counter is incremented
// and the number of pulses generated to that point is checked.
void transponder_pulse_isr(void)
{
	PWMGenIntClear(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD);

	if(g_transponder_pulse_count >= 1)
		g_transponder_times[g_transponder_pulse_count] = stopwatch_get_time_us(&g_transponder_stopwatch);	//TODO: consider if the response hasnt occured

	g_transponder_pulse_count++;
	stopwatch_start(&g_transponder_stopwatch);

	if (g_transponder_pulse_count >= TEST_ONE_NUM_PULSES)
		PWMGenDisable(PWM0_BASE, PWM_GEN_2);
}

/*-----------------------------------------------------------*/


int test_b_output_toggle(void)
{
	static int count = 0;
	static int state = 0;

	if (count >= 3 && state == 0)
	{
		state = 1;
		count = 0;

		PWMOutputState(PWM0_BASE, PWM_GEN_1_BIT, 0); //Disable PWM output
	}
	else if (count > 10 && state == 1)
	{
		state = 0;
		count = 0;

		PWMOutputState(PWM0_BASE, PWM_GEN_1_BIT, 1); // Reenable output
	}

	count++;
	return state;
}

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
	g_airspeed_pulse_count += test_b_output_toggle();
	stopwatch_start(&g_airspeed_stopwatch);

	if (g_airspeed_pulse_count >= TEST_ONE_NUM_PULSES)
		PWMGenDisable(PWM0_BASE, PWM_GEN_0);
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

// ISR responds to transponder & kernel interrupts
void transponder_response_isr(void)	//TODO: fix names
{
	GPIOPinIntClear(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	stopwatch_stop(&g_transponder_stopwatch);
	g_transponder_response_flags[g_transponder_pulse_count]++;
}

/*===================BACKGROUND TASKS========================*/


void uut_gpio_response_analysis(char response_flag, unsigned int time, int *g_out_misses,
		int *g_out_overs, int *g_out_timeouts)
{
	// make sure the ISR has occured once and only once
	if (response_flag == 0)
		++*g_out_misses;
	if (response_flag > 1)
		*g_out_overs += response_flag - 1;
	// make sure the ISR occured within a reasonable time period of the stimluls

	if (time >= TEST_ONE_MAX_RESPONSE_TIME_US && response_flag == 1)
		++*g_out_timeouts;
}

/*-----------------------------------------------------------*/

void test_one_frequency_mod(void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;
	static char direction = 0;
	static int period = TEST_ONE_MAX_PERIOD;
	for(;;)
	{
		if (direction == 0)
		{
			period-= TEST_ONE_FREQ_STEP;
			if (period < TEST_ONE_MIN_PERIOD)
				direction = 1;
		}
		else
		{
			period+= TEST_ONE_FREQ_STEP;
			if (period > TEST_ONE_MAX_PERIOD)
				direction = 0;
		}
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period*CYCLES_PER_US);
		PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, (period*CYCLES_PER_US)/2);
		vTaskDelay(iTaskDelayPeriod);
	}
}

/*-----------------------------------------------------------*/

void test_two_frequency_mod(void)
{
	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;
	static int period = TEST_TWO_INIT_PERIOD;
	for (;;)
	{
		if (period > TEST_TWO_MIN_PERIOD)
		{
			period -= TEST_TWO_FREQ_STEP;
		}

		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period*CYCLES_PER_US);
		PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, (period*CYCLES_PER_US)/2);
		vTaskDelay(iTaskDelayPeriod);
	}
}


/*===================Functions========================*/

void InitGPIO (void)
{
	// Enable GPIO port G
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

	GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_4);
	GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_PIN_4);
	reset_uut();
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


/*-----------------------------------------------------------*/

//void vTaskDisplayResults(void)
//{
//	int iTaskDelayPeriod = 500 / portTICK_RATE_MS;
//	for(;;)
//	{
//		if (g_airspeed_pulse_count >= TEST_ONE_NUM_PULSES ||
//				g_transponder_pulse_count >= TEST_ONE_NUM_PULSES)
//		{
//			RIT128x96x4Clear();
//			char buffer[20] = {0};
//			sprintf(buffer, "Test finished. /%d", (int)TEST_ONE_NUM_PULSES);
//			RIT128x96x4StringDraw(buffer, 8, 10, 4);
//			sprintf(buffer, "%d missed responses", (int)g_airspeed_misses);
//			RIT128x96x4StringDraw(buffer, 8, 20, 4);
//			sprintf(buffer, "%d extra responses", (int)g_airspeed_overs);
//			RIT128x96x4StringDraw(buffer, 8, 30, 4);
//			sprintf(buffer, "%d latency fials", (int)g_airspeed_timeouts);
//			RIT128x96x4StringDraw(buffer, 8, 40, 4);
//
//			sprintf(buffer, "%d missed responses", (int)g_transponder_misses);
//			RIT128x96x4StringDraw(buffer, 8, 60, 4);
//			sprintf(buffer, "%d extra responses", (int)g_transponder_over);
//			RIT128x96x4StringDraw(buffer, 8, 70, 4);
//			sprintf(buffer, "%d latency fails", (int)g_transponder_timeouts);
//			RIT128x96x4StringDraw(buffer, 8, 80, 4);
//
//			TimerDisable(TIMER0_BASE, TIMER_A);	//todo: create post test task to unregester interrupts etc
//			while(1);
//		}
//		vTaskDelay(iTaskDelayPeriod);
//	}
//}

/*-----------------------------------------------------------*/

//// TODO: will become test one init, or will just setup pins etc
//// and task regestration will be split off
//void uut_gpio_init(void)
//{
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0 | SYSCTL_PERIPH_PWM);
//	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
//
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);	// airspeed output
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);	// transponder output
//
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);	// airspeed response pin
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);	// transponder response pin
//
//	// Setup Airspeed & transponder GPIO
//	GPIOPinTypePWM(GPIO_PORTD_BASE, (1<<1));	// Airspeed output TODO: make pin macro
//	GPIOPinTypePWM(GPIO_PORTF_BASE, (1<<3));	// Transponder output TODO: make pin macro
//
//	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//	GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
//}
//
//void uut_gpio_test_one_init(void)
//{
//	// Register Airspeed response ISR
//	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Airspeed pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//
//
//	// Register background tasks
////	xTaskCreate( test_one_frequency_mod, "test one Frequency variation", 1000, NULL, 1, NULL);
//}
//
//void uut_gpio_test_two_init(void)
//{
//	// Register Airspeed response ISR
//	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Airspeed pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr_gpio_test_b);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//
//
////	xTaskCreate( test_two_frequency_mod, "test two Frequency variation", 1000, NULL, 1, NULL);
//}
//
//void uut_gpio_test_three_init(void)
//{
//	// Register Airspeed response ISR
//	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Airspeed pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_THREE_PERIOD*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_THREE_PERIOD*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);
//
//
//
//
//    //Register Transponder response ISR
//	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTB_BASE, &transponder_response_isr);
//	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
//
//	// Transponder pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_THREE_PERIOD*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_THREE_PERIOD*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_2, transponder_pulse_isr);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
//	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//}
//
//void uut_gpio_test_four_init(void)
//{
//	// Register Airspeed response ISR
//	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Airspeed pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_FOUR_PERIOD_A*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_FOUR_PERIOD_A*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//
//
//    //Register Transponder response ISR
//	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTB_BASE, &transponder_response_isr);
//	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
//
//	// Transponder pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FOUR_PERIOD_B*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FOUR_PERIOD_B*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_2, transponder_pulse_isr);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
//	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//}
//
//void uut_gpio_test_five_init(void)
//{
//	//Register Transponder response ISR
//	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
//	GPIOPortIntRegister(GPIO_PORTB_BASE, &transponder_response_isr);
//	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
//
//	// Transponder pulse generation
//	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
//	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
//
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FOUR_PERIOD_B*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FOUR_PERIOD_B*CYCLES_PER_US/2);
//
//	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD);
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_2, transponder_pulse_isr);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
//	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);
//}




