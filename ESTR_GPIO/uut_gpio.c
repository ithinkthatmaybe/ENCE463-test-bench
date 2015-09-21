/*
 * uut_gpio.c
 *
 *  Created on: Sep 14, 2015
 *      Author: sws52
 */



#include "uut_gpio.h"

#define CYCLES_PER_US (configCPU_CLOCK_HZ/1E6)

// Private globals
stopwatch_t g_airspeed_stopwatch;
char g_airspeed_response_flag;

stopwatch_t g_transponder_stopwatch;
char g_transponder_response_flag;

int g_airspeed_timeouts = 0;
int g_airspeed_misses = 0;
int g_airspeed_overs = 0;

int g_trans_timeouts = 0;
int g_trans_misses = 0;
int g_trans_overs = 0;


int g_airsp_pulse_count = 0;

/*-----------------------------------------------------------*/

void test_one_pulse_gen_isr(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	airspeed_pulse_generation();
}

/*-----------------------------------------------------------*/

void test_two_pulse_gen_isr(void)	// TODO: fix wierdness, looks like it should generate three pulses but it only does two, looks like theres less waits as well
												// CAUSE: its because only half of the calls to ...pulse_generation() actuall create rising edges
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	static char currently_pulsing = 1;	// indicates if currently sending a pulse train or waiting
	static int num_pulses = 0;	// used to control the number of pulses sent in a train
	static int num_waits = 0;	// used to control the number of cycles to wait before sending another train

	if (currently_pulsing)
	{
		airspeed_pulse_generation();
		num_pulses++;
		if (num_pulses > 3)
		{
			currently_pulsing = 0;
			num_waits = 0;
		}
	}
	else
	{
		num_waits++;
		if (num_waits > 10)	//TODO: make psuedo-random
		{
			currently_pulsing = 1;
			num_pulses = 0;
		}
	}
}

/*-----------------------------------------------------------*/

void test_three_pulse_gen_isr(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	airspeed_pulse_generation();
	transponder_pulse_generation();
}

/*-----------------------------------------------------------*/

void test_four_pulse_gen_isr_A(void)
{
	return;
}

/*-----------------------------------------------------------*/

void test_four_pulse_gen_isr_B(void)
{
	return;
}

/*-----------------------------------------------------------*/

void airspeed_pulse_generation(void)
{
	static int state = 0;	//State variable used to make timer generate a square wave, using a second ISR will be a better solution long term

	if (state == 0 && g_airsp_pulse_count <= TEST_ONE_NUM_PULSES)
	{

		state = 1;

		if(g_airsp_pulse_count >= 1)
		{
			// Proccess results from last pulse
			// make sure the ISR has occured once and only once
			if (g_airspeed_response_flag == 0)
				g_airspeed_misses++;
			if (g_airspeed_response_flag > 1)
				g_airspeed_overs++;

			// make sure the ISR occured within a reasonable time period of the stimluls
			unsigned long airs_time = stopwatch_get_time_us(&g_airspeed_stopwatch);
			if (airs_time >= TEST_ONE_MAX_RESPONSE_TIME_US && g_airspeed_response_flag == 1)
				g_airspeed_timeouts++;
		}

		// generate next rising edge
		g_airsp_pulse_count++;
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, UUT_AIRSPEED_OUTPUT_PIN_PE2);
		stopwatch_start(&g_airspeed_stopwatch);
		g_airspeed_response_flag = 0;
	}
	else
	{
		state = 0;
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);
	}
}


/*-----------------------------------------------------------*/

void transponder_pulse_generation(void)
{
	static int state = 0;	//State variable used to make timer generate a square wave, using a second ISR will be a better solution long term

	if (state == 0 && g_airsp_pulse_count <= TEST_ONE_NUM_PULSES)
	{
		state = 1;

		if(g_airsp_pulse_count >= 1)	// TODO: create background task to proccess, as this is lower priority
		{
			// Proccess results from last pulse
			// make sure the ISR has occured once and only once
			if (g_transponder_response_flag == 0)
				g_trans_misses++;
			if (g_transponder_response_flag > 1)
				g_trans_overs++;

			// make sure the ISR occured within a reasonable time period of the stimluls
			unsigned long trans_time = stopwatch_get_time_us(&g_transponder_stopwatch);
			if (trans_time >= TEST_ONE_MAX_RESPONSE_TIME_US && g_transponder_response_flag == 1)
				g_trans_timeouts++;
		}

		// Send next pulse
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, UUT_TRANSPONDER_OUTPUT_PIN_PE0);
		stopwatch_start(&g_transponder_stopwatch);
		g_transponder_response_flag = 0;

	}
	else
	{
		state = 0;
		GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, ~UUT_TRANSPONDER_OUTPUT_PIN_PE0);
	}
}

/*-----------------------------------------------------------*/

// ISR responds to airspeed pulses
void airspeed_response_isr(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	stopwatch_stop(&g_airspeed_stopwatch);	// TODO: protect stopwatch with mutex?
	g_airspeed_response_flag++;
//	GPIOPinWrite(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2, ~UUT_AIRSPEED_OUTPUT_PIN_PE2);	//TODO: move this line (helpful much?) meaning set this low somewhere else so we dont get stuck if the uut misses one (hackishly fixed)
}

/*-----------------------------------------------------------*/

// ISR responds to transponder & kernel interrupts
void transponder_response_isr(void)	//TODO: fix names
{
	GPIOPinIntClear(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
	stopwatch_stop(&g_transponder_stopwatch);
	g_transponder_response_flag++;
//	GPIOPinWrite(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0, ~UUT_TRANSPONDER_OUTPUT_PIN_PE0);
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
		TimerLoadSet(TIMER0_BASE, TIMER_A, period*CYCLES_PER_US/2);
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
		TimerLoadSet(TIMER0_BASE, TIMER_A, period*CYCLES_PER_US/2);
		vTaskDelay(iTaskDelayPeriod);
	}
}

/*-----------------------------------------------------------*/

void vTaskDisplayResults(void)
{
	int iTaskDelayPeriod = 500 / portTICK_RATE_MS;
	for(;;)
	{
		if (g_airsp_pulse_count > TEST_ONE_NUM_PULSES)
		{
			RIT128x96x4Clear();
			char buffer[20] = {0};
			sprintf(buffer, "Test finished. /%d", (int)TEST_ONE_NUM_PULSES);
			RIT128x96x4StringDraw(buffer, 8, 10, 4);
			sprintf(buffer, "%d missed responses", (int)g_airspeed_misses);
			RIT128x96x4StringDraw(buffer, 8, 20, 4);
			sprintf(buffer, "%d extra responses", (int)g_airspeed_overs);
			RIT128x96x4StringDraw(buffer, 8, 30, 4);
			sprintf(buffer, "%d timeouts", (int)g_airspeed_timeouts);
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
		vTaskDelay(iTaskDelayPeriod);
	}
}

/*-----------------------------------------------------------*/

// TODO: will become test one init, or will just setup pins etc
// and task regestration will be split off
void uut_gpio_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	// Setup Airspeed & transponder GPIO
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_AIRSPEED_OUTPUT_PIN_PE2);
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, UUT_TRANSPONDER_OUTPUT_PIN_PE0);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);
}

void uut_gpio_test_one_init(void)
{
	uut_gpio_init();
	// register Airspeed response ISR
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	//Register pulse generation ISR
	TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A, TEST_ONE_MAX_PERIOD*CYCLES_PER_US/2); //TODO: /2 comes from the way we generate the square wave, ie two ISRs per one cycle
	TimerIntRegister(TIMER0_BASE, TIMER_A, test_one_pulse_gen_isr);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);

	// Register background tasks
	xTaskCreate( vTaskDisplayResults, "Finished", 1000, NULL, 1, NULL);
	xTaskCreate( test_one_frequency_mod, "Frequency variation", 1000, NULL, 1, NULL);
}

void uut_gpio_test_two_init(void)
{
	uut_gpio_init();
	// register Airspeed response ISR
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	//Register pulse generation ISR
	TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A, TEST_TWO_INIT_PERIOD*CYCLES_PER_US/2); //TODO: /2 comes from the way we generate the square wave, ie two ISRs per one cycle
	TimerIntRegister(TIMER0_BASE, TIMER_A, test_two_pulse_gen_isr);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);

	// Register background tasks
	xTaskCreate( vTaskDisplayResults, "Finished", 1000, NULL, 1, NULL);
	xTaskCreate( test_two_frequency_mod, "Frequency variation", 1000, NULL, 1, NULL);
}


void uut_gpio_test_three_init(void)
{
	uut_gpio_init();
	// register Airspeed response ISR
	GPIOIntTypeSet(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTE_BASE, &airspeed_response_isr);
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	//Setup register response ISR
	GPIOIntTypeSet(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3, GPIO_RISING_EDGE);
	GPIOPortIntRegister(GPIO_PORTB_BASE, &transponder_response_isr);
	GPIOPinIntEnable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	//Register pulse generation ISR
	TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A, TEST_THREE_PERIOD*CYCLES_PER_US/2); //TODO: /2 comes from the way we generate the square wave, ie two ISRs per one cycle
	TimerIntRegister(TIMER0_BASE, TIMER_A, test_three_pulse_gen_isr);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);

	// Register background tasks
	xTaskCreate( vTaskDisplayResults, "Finished", 1000, NULL, 1, NULL);
}


