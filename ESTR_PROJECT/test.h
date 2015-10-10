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

// TEST CONFIG // TODO: move to test config file

 // Message to send. Maximum 50 characters.
#define UA_MESSAGE "`123456789012345678901234567890`456"
// Expected response. Maximum 400 characters.
#define UA_RESPONSE "ECHO ON123456789012345678901234567890ECHO OFFNACKNACKNACK";


#define GPIO_A_MAX_PERIOD_US 1000  //undefined unit
#define GPIO_A_MIN_PERIOD_US 600   //undefined unit
#define GPIO_A_FREQ_STEP_US 20

#define GPIO_B_INIT_PERIOD_US 1000
#define GPIO_B_MIN_PERIOD_US 800
#define GPIO_B_FREQ_STEP_US 50

#define GPIO_C_PERIOD_US 600

#define GPIO_D_PERIOD_US_A 600
#define GPIO_D_PERIOD_US_B 599

//#define GPIO_E_PERIOD_MS 30
//#define GPIO_E_NUM_PULSES 1


// General parameters
#define RESULTS_ID_LEN 50


// Mutual exclusion of tests
xSemaphoreHandle xTEST_DONE;

// Enable and configure peripherals used for testing
void test_init(void); 


// UART test tasks
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

void vUART_int_manage(void);
xTaskHandle xUART_int_manage;

xQueueHandle xToTest;
xQueueHandle xToTimeout;

// GPIO test tasks
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



// TEST managagement
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


void vTest_wait_for_send(int numSent);

#endif /* TEST_H_ */
