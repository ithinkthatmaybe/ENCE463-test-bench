/*
 * test.h
 *
 *  Created on: Sep 30, 2015
 *      Author: sws52
 */

#ifndef TEST_H_
#define TEST_H_

#include "stopwatch.h"
#include "uut_gpio.h"
#include "UART.h" //uut_uart
#include "PC_UART.h"

// CONFIG Parameters


#define TEST_ONE_MAX_PERIOD_US 600  //undefined unit
#define TEST_ONE_MIN_PERIOD_US 300   //undefined unit
#define TEST_ONE_FREQ_STEP_US 100

#define TEST_TWO_INIT_PERIOD_US 600
#define TEST_TWO_MIN_PERIOD_US 100
#define TEST_TWO_FREQ_STEP_US 50

#define TEST_THREE_PERIOD_US 600

#define TEST_FOUR_PERIOD_US_A 600
#define TEST_FOUR_PERIOD_US_B 599

#define TEST_FIVE_PERIOD_MS 30
#define TEST_FIVE_NUM_PULSES 5


void test_init(void); // Enable and configure peripherals used for testing

/// UART tests

void vMirrorTX( void );
xTaskHandle xMirrorTX;
void vClockSpeed( void );
xTaskHandle xClockSpeed;
void vTimeout( void );
xTaskHandle xTimeout;
void vStatus( void );
xTaskHandle xStatus;
void vEmergStatus( void );
xTaskHandle xEmergStatus;

// GPIO tests
void vGPIO_a(void);
xTaskHandle xGPIO_a;
void vGPIO_b(void);
xTaskHandle xGPIO_b;
void vGPIO_c(void);
xTaskHandle xGPIO_c;
void vGPIO_d(void);
xTaskHandle xGPIO_d;
void vGPIO_e(void);
xTaskHandle xGPIO_e;


xQueueHandle xToTest;
xQueueHandle xToTimeout;

void test_uart_a_startup(void);  // Startup function registers ISRs and Registers tasks and enables specific interupts
void test_uart_a_shutdown(void); // Undoes startup function

void test_uart_b_startup(void);
void test_uart_b_shutdown(void);

void test_uart_ci_startup(void);
void test_uart_ci_shutdown(void);

void test_uart_cii_startup(void);
void test_uart_cii_shutdown(void);

void test_uart_d_startup(void);
void test_uart_d_shutdown(void);

// GPIO tests

void test_gpio_a_startup(void);
void test_gpio_a_shutdown(void);

void test_gpio_b_startup(void);
void test_gpio_b_shutdown(void);

void test_gpio_c_startup(void);
void test_gpio_c_shutdown(void);

void test_gpio_d_startup(void);
void test_gpio_d_shutdown(void);

void test_gpio_e_startup(void);
void test_gpio_e_shutdown(void);

#endif /* TEST_H_ */
