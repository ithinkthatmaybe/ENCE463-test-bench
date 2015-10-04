/*
 * test.c
 *
 *  Created on: Sep 30, 2015
 *      Author: sws52
 */

#include "test.h"



#define UART_BUFF_LEN 100

#define RESULTS_ID_LEN 50


// TODO: will become test one init, or will just setup pins etc
// and task regestration will be split off
void test_init(void)
{
	// UART initialisatoin

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
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMIntDisable(PWM0_BASE, PWM_GEN_2);
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN); // TODO: setup sync settings
	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_LOAD); // configure pwm for end-of-cycle interrupt
	PWMGenIntRegister(PWM0_BASE, PWM_GEN_2, transponder_pulse_isr);
}

void test_gpio_a_startup(void)
{
//	// Register Airspeed response ISR
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Configure airspeed PWM
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US/2); // 50% duty
//
//	// Enable PWM output
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//
//	// Register background tasks
////	xTaskCreate( test_one_frequency_mod, "test one Frequency variation", 1000, NULL, 1, NULL);
//	xTaskCreate( test_send_results, "return results", 240, NULL, 1, NULL);
	xTaskCreate(vGPIO_a, "gpio_test_a", 240, NULL, 2, &xGPIO_a);
}

void test_gpio_a_shutdown(void)
{
//	// Disable response isr
//	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Disable PWM outputs
//	PWMGenDisable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 0);
//	xTaskCreate( xGPIO_a, 'gpio_test_a', 240, NULL, 1);
}

void test_gpio_b_startup(void)
{
	// Register Airspeed response ISR
	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);

	// Airspeed pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MIN_PERIOD_US*CYCLES_PER_US/2);

	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr_gpio_test_b);

	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);

	//register background tasks
	xTaskCreate( test_two_frequency_mod, "test two Frequency variation", 1000, NULL, 1, NULL);
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

	SysCtlPWMClockSet(SYSCTL_PWMDIV_32);

	uut_gpio_set_num_pulses(TEST_FIVE_NUM_PULSES);

	// Transponder pulse generation
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, TEST_FIVE_PERIOD_MS*CYCLES_PER_MS/8);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, TEST_FIVE_PERIOD_MS*CYCLES_PER_MS/8/2);

	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 1);

	//TODO create uart monitoring task
	xTaskCreate( test_task_e_uart_monitor, "test two Frequency variation", 240, NULL, 1, NULL);
}



void test_gpio_e_shutdown(void)
{
	// Disable response isr
	GPIOPinIntDisable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
	GPIOPinIntDisable(GPIO_PORTB_BASE, UUT_TRANSPONDER_RESPONSE_PIN_PB3);

	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

	uut_gpio_set_num_pulses(MAX_NUM_PULSES);

	// Disable PWM outputs
	PWMGenDisable(PWM0_BASE, PWM_GEN_2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, 0);
}



void test_task_e_uart_monitor(void)
{
	char buffer[UART_BUFF_LEN];
	char buff;
	int i = 0;
	for (;;)
	{

		while(xQueueReceive(xUARTReadQueue, &buff, (portTickType)10))
		{
			buffer[i++] = buff;
		}

	}
}


void vGPIO_a(void)
{
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

	int iTaskDelayPeriod = 50 / portTICK_RATE_MS;
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
		vTaskDelay(iTaskDelayPeriod);


		if (g_airspeed_pulse_count >= MAX_NUM_PULSES) // Test finished
		{
			airspeed_response_results.test_type = '1';
			airspeed_response_results.test_data = &g_airspeed_response_flags;
			airspeed_response_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_response_results.test_string = &airspeed_response_id;
			airspeed_response_results.test_string_len = strlen(airspeed_response_id);

			airspeed_latency_results.test_type = '1';
			airspeed_latency_results.test_data = &g_airspeed_times;
			airspeed_latency_results.num_of_elements = MAX_NUM_PULSES;
			airspeed_latency_results.test_string = &airspeed_latency_id;
			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);

			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);

			while(1)
			{
				vTaskDelay(500);
			}

		}

	}
}

//void vGPIO_b (void)
//{
//	// Register Airspeed response ISR
//	GPIOPinIntEnable(GPIO_PORTE_BASE, UUT_AIRSPEED_RESPONSE_PIN_PE3);
//
//	// Airspeed pulse generation
//	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, TEST_ONE_MAX_PERIOD_US*CYCLES_PER_US);
//	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, TEST_ONE_MIN_PERIOD_US*CYCLES_PER_US/2);
//
//	PWMGenIntRegister(PWM0_BASE, PWM_GEN_0, airspeed_pulse_isr_gpio_test_b);
//
//	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
//	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, 1);
//
//
//	for (;;)
//	{
//		// End of test
//		if (g_airspeed_pulse_count >= MAX_NUM_PULSES)
//		{
//			airspeed_response_results.test_type = '1';
//			airspeed_response_results.test_data = &g_airspeed_response_flags;
//			airspeed_response_results.num_of_elements = MAX_NUM_PULSES;
//			airspeed_response_results.test_string = &airspeed_response_id;
//			airspeed_response_results.test_string_len = strlen(airspeed_response_id);
//
//			airspeed_latency_results.test_type = '1';
//			airspeed_latency_results.test_data = &g_airspeed_times;
//			airspeed_latency_results.num_of_elements = MAX_NUM_PULSES;
//			airspeed_latency_results.test_string = &airspeed_latency_id;
//			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);
//
//			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
//			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);
//
//			while(1)
//			{
//				vTaskDelay(500);
//			}
//	}
//
//}













//
//void test_send_results(void)
//{
//	int iTaskDelayPeriod = 500 / portTICK_RATE_MS;
//
//	Test_res airspeed_response_results = {NULL,NULL,NULL,NULL,NULL};
//	Test_res* airspeed_response_results_ptr = &airspeed_response_results;
//	char airspeed_response_id[RESULTS_ID_LEN] = "airspeed_response_flags\0";
//
//	Test_res airspeed_latency_results = {NULL, NULL, NULL, NULL, NULL};
//	Test_res* airspeed_latency_results_ptr = &airspeed_latency_results;
//	char airspeed_latency_id[RESULTS_ID_LEN] = "airspeed_latency\0";
//
//	for (;;)
//	{
//		if (g_airspeed_pulse_count >= MAX_NUM_PULSES)
//		{
//			airspeed_response_results.test_type = '1';
//			airspeed_response_results.test_data = &g_airspeed_response_flags;
//			airspeed_response_results.num_of_elements = MAX_NUM_PULSES;
//			airspeed_response_results.test_string = &airspeed_response_id;
//			airspeed_response_results.test_string_len = strlen(airspeed_response_id);
//			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_response_results_ptr, (portTickType)10);
//
//			airspeed_latency_results.test_type = '1';
//			airspeed_latency_results.test_data = &g_airspeed_times;
//			airspeed_latency_results.num_of_elements = MAX_NUM_PULSES;
//			airspeed_latency_results.test_string = &airspeed_latency_id;
//			airspeed_latency_results.test_string_len = strlen(airspeed_latency_id);
//			xQueueSendToBack( xSEND_RESULTS_Queue, (void*)&airspeed_latency_results_ptr, (portTickType)10);
//			while(1);
//
//		}
//		vTaskDelay(iTaskDelayPeriod);
//	}
//}













