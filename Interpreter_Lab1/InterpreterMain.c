// UARTIntsTestMain.c
// Runs on LM4F120/TM4C123
// Tests the UART0 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 5.11 Section 5.6, Program 3.10

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include <stdint.h>
#include "PLL.h"
#include "UART.h"
#include "OS.h"
#include "ST7735TestResources.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

//---------------------UART_NewLine---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void UART_NewLine(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void print_prompt(){
  UART_NewLine(); UART_NewLine();
  UART_OutString("Please input one of the following numbers:");
	UART_NewLine();
	UART_OutString("1-Show ISR Count 2-Read ADC 3-Graphics Test");
	UART_NewLine();

}

int interpreter(void){
	uint32_t n;
	char string[20];  // global to assist in debugging
	print_prompt();
  while(1){
    UART_OutString(">>> ");
    n = UART_InUDec();
		switch(n){
			case(1):
				UART_NewLine();
				sprintf(string, "%lu", OS_ReadPeriodicTime());
				UART_OutString(string);
				break;
			case(2):
				UART_NewLine();
				UART_OutString("Feature not available!");
				break;
			case(3):
				full_test();
				break;
		}
		print_prompt();
	}
}

int main(void){
	PLL_Init(Bus80MHz);         // bus clock at 80 MHz
  Output_Init();  // initializes LCD
  UART_Init();    // initialize UART
	OS_AddPeriodicThread(dummy, 80000, 3);
  interpreter();
	while (1) {
	}
}
