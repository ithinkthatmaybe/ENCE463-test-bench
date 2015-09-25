/*
 * PC_UART.h
 *
 *  Created on: Sep 23, 2015
 *      Author: jwl52
 */

#ifndef PC_UART_H_
#define PC_UART_H_

xQueueHandle xPC_UARTReadQueue;
void send_results_to_PC(const unsigned char *outBuffer, unsigned long ulCount);


#endif /* PC_UART_H_ */
