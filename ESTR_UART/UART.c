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
    ulStatus = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ulStatus);


    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART0_BASE))
    {
        // Read the next character from the UART and write it back to the UART.
        buff = UARTCharGetNonBlocking(UART0_BASE);
    	//RIT128x96x4Clear();
    	xQueueSendFromISR(xUARTReadQueue, (void*)&buff, pdFALSE );
    }
}

void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
    // Loop while there are more characters to send.
    while(ulCount--)
    {
        // Write the next character to the UART.
        UARTCharPutNonBlocking(UART0_BASE, *pucBuffer++);
    }
}

void InitUART (void)
{
	// Enable UART0 and GPIO Port A.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Set GPIO A0 and A1 as UART pins.
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);


	 // Configure the UART for 9600, 8-N-1 operation.
	 UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
						 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
						  UART_CONFIG_PAR_EVEN));

	 // Register and enable the UART interrupt.
	 UARTEnable(UART0_BASE);
	 UARTIntRegister(UART0_BASE, UARTIntHandler);
	 IntEnable(INT_UART0);
	 UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	 // Ceate required queue for reading from UART
	 xUARTReadQueue = xQueueCreate(20, sizeof(char));
}
