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





