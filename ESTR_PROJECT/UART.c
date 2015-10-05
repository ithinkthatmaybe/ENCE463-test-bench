/*
 * UART.c
 *
 *  Created on: Sep 13, 2015
 *      Author: Ben Litchfield and Jack Linton
 */

#include "UART.h"
#include "PC_UART.h"


stopwatch_t UART_stopwatch;




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
UART_GPIO_IntHandler(void)
{
    //unsigned long ulStatus;

    // Get the interrrupt status.
    //ulStatus = GPIOPinIntStatus(GPIO_PORTC_BASE, true);
    // Clear the asserted interrupts.
    GPIOPinIntClear(GPIO_PORTH_BASE, GPIO_PIN_2 );

    if (GPIOPinRead(GPIO_PORTH_BASE,GPIO_PIN_2 ) == GPIO_PIN_2){
    	stopwatch_start(&UART_stopwatch);
    }
    else
    {
    	int time = 1000 * stopwatch_get_time_ms(&UART_stopwatch);
		time += stopwatch_get_time_us(&UART_stopwatch);

		xQueueSendFromISR(xUART_int_queue, (void*)&time, pdFALSE );
    // Loop while there are characters in the receive FIFO.
    }

}


void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount, unsigned long ulBase)
{


	//stopwatch_t stopwatch;



	int i = 0;
    // Loop while there are more characters to send.
    while(ulCount--)
    {
        // Write the next character to the UART.
        UARTCharPutNonBlocking(ulBase, *pucBuffer++);
        // Delay in order to be able to send long strings over UART.
        while (i < 10000)
        {
        	i++;
        }
        i = 0;
    }
}

// Performs a mirror test.
void mirrorUART(unsigned char *mirrorMessage, unsigned long ulCount, unsigned long ulBase, int exLen, char * out)
{
	// Variables
	int len = ulCount;
	int charsSent = 0;
	unsigned char * prev = mirrorMessage;
	char cReceived;
	int i = 0;
	int j = 0;
	char  buffer[100];// = (char *) calloc (exLen, sizeof(char)); // Temporary buffer for results.
	int waiting = 0;
	int done = 0;


	while(!done)
	{
		if (!waiting)
		{
			while (charsSent < len)
			{
				// Split messages around ` characters, as the UUT takes a small amount of time to change mode.
				if (mirrorMessage[charsSent] == '`')
				{
					if (charsSent > 0)
					{
						UARTSend((unsigned char *)prev, &mirrorMessage[charsSent] - prev, UART1_BASE);
					}
					UARTSend((unsigned char *)"`", sizeof(char), UART1_BASE);
					charsSent++;
					prev = &mirrorMessage[charsSent];
					break;
				}
				charsSent++;
			}
			if (charsSent == len)
			{
				// This sends the last portion of the message when the message does not end with a `.
				UARTSend((unsigned char *)prev, &mirrorMessage[charsSent] - prev, UART1_BASE);
				charsSent++;
			}
			waiting = 1;
		}
		if (waiting)
		{
			if (xUARTReadQueue !=0)
			{
				while (xQueueReceive(xUARTReadQueue, &cReceived, (portTickType)10))
				{
					if (cReceived == '|') // Timeout character. Finish test.
					{
						done = 1;
						while (j < exLen){
							*(out+j) = buffer[j];
							j++;
						}
						//free(buffer);
						break;
					}

					buffer[i++] = cReceived;
					waiting = 0;

					if (i == exLen) // Message received. Write message to results.
					{
						while (j < i){
							*(out+j) = buffer[j];
							j++;
						}
						done = 1;
						//free(buffer);
					}
				}
			}
		}
	}
}



void InitUART (void)
{


	//GPIOPinConfigure(GPIO_PA0_U0RX);



	// Enable GPIO for UART fault detection

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_2);
	GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_2,  GPIO_BOTH_EDGES );
	GPIOPortIntRegister(GPIO_PORTH_BASE, UART_GPIO_IntHandler); // was &airspeed_response_isr
	GPIOPinIntEnable(GPIO_PORTH_BASE, GPIO_PIN_2);

	//IntEnable(INT_UART1);
	//IntMasterEnable();


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
//	 UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
//						 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
//						  UART_CONFIG_PAR_EVEN));

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

	 // Create required queue for reading from UART
	 xUARTReadQueue = xQueueCreate(40, sizeof(char));
	 xUART_int_queue = xQueueCreate(10, sizeof(int));
	 stopwatch_start(&UART_stopwatch);
}
