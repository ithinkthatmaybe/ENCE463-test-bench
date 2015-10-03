/*
 * PC_UART.h
 *
 *  Created on: Sep 23, 2015
 *      Author: bel40
 *      Don't forget to add the prototype  "void send_results_to_PC(void);" and
 *      "void Monitor_PC_UART(void);"  in the main file
 *
 *      You will also need to exectute Init_PC_UART (void)
 */

#ifndef PC_UART_H_
#define PC_UART_H_
#include "UART.h"
#include "include/task.h"
#include "include/semphr.h"
#include "stdio.h"
#include "string.h"
struct Test_results

{
	char test_type;
	int num_of_elements;
	int* test_data;
	char* test_string;
	int test_string_len;
};// Test_results_default = {NULL,NULL,NULL,NULL,NULL};

typedef struct Test_results Test_res;


xQueueHandle xSEND_RESULTS_Queue;
xQueueHandle xCOMMS_FROM_PC_Queue;
xSemaphoreHandle xPC_SENT;

void Init_PC_UART (void);
void send_results_to_PC();


#endif /* PC_UART_H_ */
