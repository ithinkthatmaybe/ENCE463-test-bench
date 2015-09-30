//PC Coms module

#include "PC_UART.h"
#include "drivers/rit128x96x4.h"

//*****************************************************************************
//
// The PC_UART interrupt handler.
//
//*****************************************************************************
void
PC_UARTIntHandler(void)
{
    unsigned long ulStatus;
    char buff;

    // Get the interrrupt status.
    ulStatus = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ulStatus);


    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART0_BASE))
    {
        // Read the next character from the UART and write it back to the UART.
        buff = UARTCharGetNonBlocking(UART0_BASE);
    	//RIT128x96x4Clear();
    	xQueueSendFromISR(xCOMMS_FROM_PC_Queue, (void*)&buff, pdFALSE );
    }
}


void Init_PC_UART (void)
{
	// Enable UART0 and GPIO Port A.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Set GPIO A0 and A1 as UART pins.
	GPIOPinConfigure(GPIO_PA0_U0RX);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);


	 // Configure the UART for 9600, 8-N-1 operation.
	 UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115000,
						 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
						  UART_CONFIG_PAR_EVEN));

	 // Register and enable the UART interrupt.
	 UARTEnable(UART0_BASE);
	 UARTIntRegister(UART0_BASE, PC_UARTIntHandler);
	 IntEnable(INT_UART0);
	 UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	 // Create required queue for sending results to PC result sender for transmission via UART
	 xSEND_RESULTS_Queue = xQueueCreate(10, sizeof(struct Test_results));
	 xCOMMS_FROM_PC_Queue = xQueueCreate(20, sizeof(char));
	 // create pc sending task
	 xTaskCreate(send_results_to_PC, "Task 4", 240, NULL, 1, NULL );

	 // used for displaying text from PC sent by UART. For testing purposes only


}

//void Monitor_PC_UART()
//{
//	// Monitors the  PC receive queue (UART chanel 0) and prints 10 characters on the screen of the stellaris.
//	// If more than 10 chars are received the screen is reset and printing begins again.
//	char cReceived;
//	int i = 0;
//	char buffer[20] = {0};
//
//	for( ;; )
//	{
//		if (xCOMMS_FROM_PC_Queue !=0)
//		{
//			if (xQueueReceive(xCOMMS_FROM_PC_Queue, &cReceived, (portTickType)10))
//			{
//				if (cReceived == 'm')
//				{
//
//					RIT128x96x4StringDraw("Hello Martin", 5, 10, 30);
//				}
//			}
//
//		}
//
//	}
//}



void send_results_to_PC()
{
	int i = 0;
	int temp = 0;
	char charbuff[10];
	struct Test_results*  results;

	while (1)
	{	// checks if queue exists
		if (xSEND_RESULTS_Queue !=0)
		{
			while (xQueueReceive(xSEND_RESULTS_Queue, &(results), (portTickType)10))
			{
				UARTCharPutNonBlocking(UART0_BASE, (*results).test_type);
				// semi colon used to indicate start of data array
				UARTCharPutNonBlocking(UART0_BASE, ';');

				temp = (*results).num_of_elements;
				if ((*results).test_data != NULL)
				{
			       for (i = 0; i <  temp; i++)
			       {
			        // convert  each long to a string
			        sprintf(charbuff, "%d", *((*results).test_data+(i)));

			        // iterate through string sending each char to PC via UART0
			        UARTSend(charbuff, strlen(charbuff),UART0_BASE );

			        	if (i < temp-1)
			        	{
			        		// inserts , between each data item
			        		UARTSend(",", 1,UART0_BASE );
			        		vTaskDelay(1);
			        	}
			        	// inserts semi colon to indicate end of data array
			        	else UARTSend(";", 1,UART0_BASE );
			       }

			        if( i < (*results).num_of_elements-1)
			        {
			        	UARTCharPutNonBlocking(UART0_BASE, ',');    ////   Replace putchar with uart send character command.
			        }
				}
			        // Checks if text to send
				if ((*results).test_string != NULL)
			    {
					// Send text.
					UARTSend((*results).test_string, (*results).test_string_len,UART0_BASE ); // sends text
					// send semicolon to indicate end of text
					UARTSend(";", 1,UART0_BASE );

				}
			}
		}
	}
}

