// Interpreter.h
// Adapted from UARTInts_4C123.c
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

#define BUFFERSIZE 100
#include <stdint.h>
#include "PLL.h"
#include "UART.h"
#include "OS.h"
#include "ST7735TestResources.h"
#include "ADCT0ATrigger.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
int32_t StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(int32_t sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

unsigned short buf[BUFFERSIZE];

//---------------------UART_NewLine---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void UART_NewLine(void) {
  UART_OutChar(CR);
  UART_OutChar(LF);
}

void print_help() {
	UART_NewLine();
	UART_OutString("disp string : Prints string to the display at line 1 on device 1");
	UART_NewLine();
	//UART_OutString("disp2 -string- : Prints -string- to the display at line 1 on device 2");
	//UART_NewLine();
	UART_OutString("isr : Prints the ISR Count to the shell");
	UART_NewLine();
	UART_OutString("adc : Runs ADC_Collect and prints the values to the shell");
	UART_NewLine();
	UART_OutString("gtest : Runs a graphics test");
	UART_NewLine();
}

void print_prompt() {
  //UART_NewLine(); UART_NewLine();
  //UART_OutString("Please input one of the following numbers:");
	//UART_NewLine();
	//UART_OutString("1-Show ISR Count 2-Read ADC 3-Graphics Test");
	UART_NewLine();
	UART_OutString(">>> ");

}

int check_string(char* strptr) {
	int retv = 0;
	if(strptr[0] == 'd' && 
		 strptr[1] == 'i' && 
	   strptr[2] == 's' && 
	   strptr[3] == 'p') {
		if( strptr[4] == '1') {
			retv = 4;
		} else  {
			retv = 4;
		}
	} else if (strptr[0] == 'i' && 
		         strptr[1] == 's' && 
	           strptr[2] == 'r') {
		retv = 1; 
	} else if (strptr[0] == 'a' && 
		         strptr[1] == 'd' && 
	           strptr[2] == 'c') {
		retv = 2; 
	} else if (strptr[0] == 'g' && 
		         strptr[1] == 't' && 
	           strptr[2] == 'e' && 
	           strptr[3] == 's' && 
	           strptr[4] == 't') {
		retv = 3;
	} else if (strptr[0] == 'h' && 
		         strptr[1] == 'e' && 
	           strptr[2] == 'l' && 
	           strptr[3] == 'p') {
		retv = 6;
	} else {
		retv = 0;
	}
	return retv;
}

void print_adc(char* string) {
	/*for(int i = 0; i < 1000; i++) {
		sprintf(string, "%u ", ADC_In());
		UART_OutString(string);
		if(i % 10 == 9) {
			UART_NewLine();
		}
	}
	ADC_Collect(11, 1000, buf, BUFFERSIZE);
	while(ADC_Stop() != 0) {
	}
	for(int i = 0; i < BUFFERSIZE; i++) {
		sprintf(string, "%u ", buf[i]);
		UART_OutString(string);
		if(i % 10 == 9) {
			UART_NewLine();
		}
	}*/
}

void Interpreter(void) {
	uint32_t n = 7;
	char string[20];  // global to assist in debugging
	char input_string[32];
  while(1){	
		print_prompt();
		UART_InString(input_string, 30);
		n = check_string(input_string);
    //n = UART_InUDec();
		UART_NewLine();
		
		switch(n){
			case(0):
				UART_OutString("Command Not Recognized. Type help for possible commands");
				break;
			case(1):
				sprintf(string, "%lu", OS_ReadPeriodicTime());
				UART_OutString(string);
				break;
			case(2):
				print_adc(string);
				break;
			case(3):
				full_test();
				break;
			case(4):
				Output_Clear();
				ST7735_Message(1, 1, &input_string[5], 0);
			case(5):
				UART_OutString(&input_string[5]);
				break;
			case(6):
				print_help();
				break;
			case(7):
				UART_OutString("YOU DONE F'D UP");
				break;
		}
	}
}

