/*
 * UART.h
 *
 *  Created on: Sep 13, 2015
 *      Author: Ben Litchfield and Jack Linton
 *      Description: 	Contains functions to utilise the UART
 *      				connection between the ESTR and the UUT.
 *      				Interrupt implemented in UART.c sends received
 *      				characters to xUARTReadQueue.
 */

#ifndef UART_H_
#define UART_H_

#include "inc/lm3s1968.h"
#include "inc\hw_types.h"
#include "inc\hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "include/FreeRTOS.h"
#include "include/queue.h"
#include "driverlib/pin_map.h"

//#include "UART.c"

//Definitions from pin_map.h, which isn't being included properly for some reason.
//ERROR NEEDS FIXING, THIS IS NOT HOW THESE SHOULD BE INCLUDED
#define GPIO_PA0_U0RX           0x00000001
#define GPIO_PA1_U0TX           0x00000401
#define GPIO_PD2_U1RX           0x00030801
#define GPIO_PD3_U1TX           0x00030C01

xQueueHandle xUARTReadQueue;

//Sends given message over UART0 channel. Requires message and length of message.
void UARTSend(const unsigned char *pucBuffer, unsigned long ulCount);

//Initialises UART0 including all required pins and interrupts for mirror function.
void InitUART (void);

#endif /* UART_H_ */
