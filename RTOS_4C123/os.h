// os.h
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

   Programs 4.4 through 4.12, section 4.2

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


#ifndef __OS_H
#define __OS_H  1

// fill these depending on your clock
#define TIME_1MS  50000
#define TIME_2MS  2*TIME_1MS

#define NUMTHREADS  3        // maximum number of threads
#define STACKSIZE   100      // number of 32-bit words in stack


#include <stdint.h>
#include "PLL.h"
#include "inc/tm4c123gh6pm.h"
#include <stdbool.h>
#include <assert.h>
#include "Timers.h"

#define NVIC_ST_CTRL_R          (*((volatile uint32_t *)0xE000E010))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_R        (*((volatile uint32_t *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile uint32_t *)0xE000E018))
#define NVIC_INT_CTRL_R         (*((volatile uint32_t *)0xE000ED04))
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // Set pending SysTick interrupt
#define NVIC_SYS_PRI3_R         (*((volatile uint32_t *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority

// function definitions in osasm.s
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
int32_t StartCritical(void);
void EndCritical(int32_t primask);
void StartOS(void);
void ContextSwitch(void);

bool timer_occupied[4] = {false};
int (*timer_init_fns[4]) (void(*task)(void), unsigned long period);

struct tcb{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
	uint64_t sleep_alarm; // set to 0 when not sleeping, otherwise equal to OS_ISR_Count to wake up on
	int16_t priority;  // higher is more important, -1 means unallocated
};
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;


void tcb_set_empty(tcbType *tcbobj){
	tcbobj->priority = -1;
}

bool tcb_is_empty(tcbType tcbobj){
	// by empty, i mean hosting a null thread
	// use priority = -1 as a flag to indicate
	// a slot in tcbs that doesn't hold an active
	// thread
	return tcbobj.priority == -1;
}	

bool tcbs_all_empty(){
	for (uint16_t i = 0; i < NUMTHREADS; i++){
		if (!tcb_is_empty(tcbs[i])){
			return false;
		}
	}
	return true;
}

bool tcbs_all_full(){
	for (uint16_t i = 0; i < NUMTHREADS; i++){
		if (tcb_is_empty(tcbs[i])){
			return false;
		}
	}
	return true;
}

int32_t Stacks[NUMTHREADS][STACKSIZE];

void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14  
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12	 
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}

uint64_t OS_ISR_period = 50000;
uint16_t OS_ISR_priority = 3;
uint64_t OS_ISR_Count = 0;
/********* OS_Suspend **************
Stop execution of currently active
foreground thread. Move on to next.
************************************/
void OS_Suspend(void){  // TODO: Deal with sleep
	NVIC_ST_CURRENT_R = 0;  // clear counter
	NVIC_INT_CTRL_R = 0x04000000;  // trigger systick
}
/********** OS_Wait ************
*******************************/
void OS_Wait(int32_t *s){
	DisableInterrupts();
	while(*s <= 0){
		EnableInterrupts();
		OS_Suspend();
		DisableInterrupts();
	}
	*s = *s - 1;
	EnableInterrupts();
}


/********** OS_ISR *************
what to run on systick interrupt
********************************/
void OS_ISR(void);

bool preemptive_mode;  // need to remember mode
// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: systick, 50 MHz PLL
// input:  bool preemptive - whether to init isr
// output: none
void OS_Init(bool preemptive){
  OS_DisableInterrupts();
	preemptive_mode = preemptive;
  PLL_Init(Bus50MHz);         // set processor clock to 50 MHz
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
	for (uint16_t i = 0; i < NUMTHREADS; i++){
		tcbs[i].priority = -1;
	}
	OS_ISR_Count = 0;
	SysTick_Init(OS_ISR_period, 
								 OS_ISR_priority);	
	timer_init_fns[0] = Timer0A_Init;
	timer_init_fns[1] = Timer1A_Init;
	timer_init_fns[2] = Timer2A_Init;
	timer_init_fns[3] = Timer3A_Init;
}

//******** OS_AddThread ***************
// add three foregound threads to the scheduler
// Inputs: three pointers to a void/void foreground tasks
// Outputs: 1 if successful, 0 if this thread can not be added
bool OS_AddThread(void(*task)(void), uint16_t priority){ 
	int32_t status;
  status = StartCritical();
	if (tcbs_all_full()){ return false; } // no room
	tcbType new_next;
	if (!tcbs_all_empty()){ // new thread skips to front of line
		tcbType new_next = *(RunPt->next);  // this could be problematic
	}  // if a thread keeps adding itself, worry about that later
	int16_t new_tcb_index = -1;
	bool found_free = false;
	while(!found_free && new_tcb_index < 3){
		new_tcb_index++;
		found_free = tcb_is_empty(tcbs[new_tcb_index]);
	}
	SetInitialStack(new_tcb_index);
	tcbs[new_tcb_index].sleep_alarm = 0;
	Stacks[new_tcb_index][STACKSIZE-2] = (int32_t)(task);
	if (tcbs_all_empty()){ // no threads added yet
		RunPt = &tcbs[new_tcb_index]; // this thread runs first
		tcbs[new_tcb_index].next = &tcbs[new_tcb_index];  // points to self
	} else { // insert after current running thread
		tcbs[new_tcb_index].next = RunPt->next;  // new thread points to next
		RunPt->next = &tcbs[new_tcb_index];  // current thread points to new
	}
	tcbs[new_tcb_index].priority = priority;
  EndCritical(status);
  return true;               // successful
}

//******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(uint32_t theTimeSlice){
	if (preemptive_mode){
		NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
		NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
  }
	StartOS();                   // start on the first task
}
								 
/********* OS_Sleep ****************
Mark currently active foreground
thread as sleeping for clock cycles
passed in as argument, then suspend
***********************************/
void OS_Sleep(uint64_t time){
	assert(preemptive_mode);  // sleep is implemented using OS_ISR_Count, so if no ISRs sleep doesn't work
	RunPt->sleep_alarm = OS_ISR_Count + time/OS_ISR_period + 1;  // round up
	OS_Suspend();
}

/******** OS_Kill ******************
Remove current thread from linked list of
running threads
***********************************/
void OS_Kill(){
	int32_t status;
  status = StartCritical();
	tcbType prev;  // search for what points to me
	for (uint16_t i = 0; i < NUMTHREADS; i++){
		if (!tcb_is_empty(tcbs[i]) &&
			tcbs[i].next == RunPt){  // is this better than having
			prev = *tcbs[i].next;  // a doubly linked list?
		}
	}
	prev.next = RunPt->next;
	tcb_set_empty(RunPt);
	EndCritical(status);
}

bool OS_AddPeriodicThread(void(*task) (void),
													uint64_t period,
												  uint16_t priority){
  int16_t timer_to_use = -1;
	for (int16_t i = 0; i < sizeof(timer_occupied); i++){
		if(!timer_occupied[i]){
			timer_to_use = i;
			break;
		}
	}
	if (timer_to_use == -1){ return false; }  // no free timers
	timer_occupied[timer_to_use] = true;
	timer_init_fns[timer_to_use](task, period);  // TODO: set priority too
	return true;						
}

#endif
