
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "SysTickInts.h"
#define PF2     (*((volatile uint32_t *)0x40025010))

unsigned long OS_ISR_Count;
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void(*PeriodicThread) (void);  // pointer to fn to run periodically
// turns on gpio port f pin 2 to toggle for dummy

void init_dummy(){
  SYSCTL_RCGCGPIO_R |= 0x20;  // activate port F
  GPIO_PORTF_DIR_R |= 0x04;   // make PF2 output (PF2 built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x04;// disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x04;   // enable digital I/O on PF2
                              // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
}

// a function to test the minimum required time for the OS ISR
void dummy(void){
  PF2 ^= 0x04;                // toggle PF2
  PF2 ^= 0x04;                // toggle PF2
  OS_ISR_Count = OS_ISR_Count + 1;
  PF2 ^= 0x04;                // toggle PF2
}
// Clears the OS ISR Counter
void OS_ClearPeriodicTime(void){ OS_ISR_Count = 0; }

// Return the current 32-bit global counter
// units of this system time are the period of
// interrupt passed in by the user when initializing
// with OS_AddPeriodicThread
unsigned long OS_ReadPeriodicTime(void){ return OS_ISR_Count; }

void OS_ISR(void){
	PeriodicThread();
}

/* Pretty self explanatory, this initializes
 the OS ISR, for now assume that there will only be one
 active interrupt */
int OS_AddPeriodicThread(void(*task) (void), 
	unsigned long period, unsigned long priority){
		OS_ISR_Count = 0;
		 init_dummy();  // turn on gpio port f pin 2
		SysTick_Init(period, priority, OS_ISR);        // initialize SysTick timer
		EnableInterrupts();
		PeriodicThread = task;
		return 0;  // why is this function call returning an int?
}

void OS_ContextSwitch(){

}

