/*
 * UART.c
 *
 *  Created on: Sep 13, 2015
 *      Author: Ben Litchfield and Jack Linton
 */

#include "UART.h"


//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    unsigned long ulStatus;
    char buff;

    // Get the interrrupt status.
    ulStatus = UARTIntStatus(UART1_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART1_BASE, ulStatus);


    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART1_BASE))
    {
        // Read the next character from the UART and write it back to the UART.
        buff = UARTCharGetNonBlocking(UART1_BASE);
    	//RIT128x96x4Clear();
    	xQueueSendFromISR(xUARTReadQueue, (void*)&buff, pdFALSE );
    }
}

void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount, unsigned long ulBase)
{
    // Loop while there are more characters to send.
    while(ulCount--)
    {
        // Write the next character to the UART.
        UARTCharPutNonBlocking(ulBase, *pucBuffer++);
    }
}


void mirrorUART(unsigned char *mirrorMessage, unsigned long ulCount, unsigned long ulBase, int exLen, char * out)
{
	// Variables

	int len = ulCount;
	int j = 0;
	unsigned char * prev = mirrorMessage;
	char cReceived;
	int i = 0;
	char * buffer = (char *) malloc (exLen);
	int waiting = 0;
	int done = 0;

	while(!done)
	{
		if (!waiting)
		{
			while (j < len)
			{
				if (mirrorMessage[j] == '`')
				{
					if (j > 0)
					{
						UARTSend((unsigned char *)prev, &mirrorMessage[j] - prev, UART1_BASE);
					}
					UARTSend((unsigned char *)"`", sizeof(char), UART1_BASE);
					j++;
					prev = &mirrorMessage[j];
					break;
				}
				j++;
			}
			if (j == len)
			{
				UARTSend((unsigned char *)prev, &mirrorMessage[j] - prev, UART1_BASE);
				j++;
			}
			waiting = 1;
		}
		if (waiting)
		{
			if (xUARTReadQueue !=0)
			{
				while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
				{
					if (cReceived == '|')
					{
						done = 1;
						break;
					}
					buffer[i++] = cReceived;
					waiting = 0;

					if (i == exLen)
					{
						j = 0;
						while (j < i){
							*(out+j) = buffer[j];
							j++;
						}
						done = 1;
					}
				}
			}
		}
	}
}



void InitUART (void)
{
	// Enable UART0 and GPIO Port A.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	// Set GPIO A0 and A1 as UART pins.
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Set GPIO D2 and D3 as UART pins. Used for PC connection
	GPIOPinConfigure(GPIO_PD2_U1RX);
	GPIOPinConfigure(GPIO_PD3_U1TX);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);


	 // Configure the UART for 9600, 8-N-1 operation.
	 UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
						 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
						  UART_CONFIG_PAR_EVEN));

	 UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
							 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
							  UART_CONFIG_PAR_EVEN));

	 // Register and enable the UART interrupt.
	 //UARTEnable(UART0_BASE);
	 UARTEnable(UART1_BASE);
	 //UARTIntRegister(UART0_BASE, UARTIntHandler);
	 UARTIntRegister(UART1_BASE, UARTIntHandler);
	 //IntEnable(INT_UART0);
	 IntEnable(INT_UART1);
	// UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	 UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);

	 // Ceate required queue for reading from UART
	 xUARTReadQueue = xQueueCreate(20, sizeof(char));


	 xPC_UARTReadQueue = xQueueCreate(20, sizeof(char));
	 xPC_UARTWriteQueue = xQueueCreate(20, sizeof(char));

}
