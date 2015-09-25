//PC Coms module



#include "UART.h"
#include "PC_UART.h"

void Init_PC_UART (void)
{
	// Enable UART0 and GPIO Port A.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Set GPIO A0 and A1 as UART pins.
	GPIOPinConfigure(GPIO_PA0_U0RX);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);


	 // Configure the UART for 9600, 8-N-1 operation.
	 UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
						 (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
						  UART_CONFIG_PAR_EVEN));

	 // Register and enable the UART interrupt.
	 UARTEnable(UART0_BASE);
	 //UARTIntRegister(UART0_BASE, UARTIntHandler);
	 IntEnable(INT_UART0);
	 UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	 // Ceate required queue for reading from UART
	// xSEND_RESULTS_Queue = xQueueCreate(20, sizeof(char));
}



void send_results_to_PC(const unsigned char *outBuffer, unsigned long ulCount)
{


	    // Loop while there are more characters to send.
	    while(ulCount--)
	    {
	        // Write the next character to the UART.
	        UARTCharPutNonBlocking(UART0_BASE, *outBuffer++);
	    }

}



