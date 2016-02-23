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
#define TIME_500US TIME_1MS / 1000

#define NUMTHREADS  3        // maximum number of threads!
#define STACKSIZE   100      // number of 32-bit words in stack


#include <stdint.h>
#include "PLL.h"
#include "inc/tm4c123gh6pm.h"
#include <stdbool.h>
#include <assert.h>
#include "Timers.h"
#include "Switch.h"
#include "SysTickInts.h"
#include "Globals.h"

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
void (*timer_init_fns[4]) (void(*task)(void), uint32_t period, uint16_t priority);

struct Sema4{
  int16_t Value;   // >0 means free, otherwise means busy 
};
typedef struct Sema4 Sema4Type;

Sema4Type Mutex;
Sema4Type DataAvailable;
Sema4Type MailboxFull;
Sema4Type MailboxEmpty;

struct tcb{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
	uint64_t sleep_alarm; // set to 0 when not sleeping, otherwise equal to OS_ISR_Count to wake up on
	int16_t priority;  // higher is more important, -1 means unallocated
	unsigned long myID; // Needed for OS_Id()
};
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;

//Function prototyping
void OS_bSignal(Sema4Type *s);
void OS_Signal(Sema4Type *s);
void OS_Wait(Sema4Type *s);
void OS_bWait(Sema4Type *s);
void OS_InitSemaphore(Sema4Type *semaPt, uint16_t value);

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

//SWITCH INTERRUPT HANDLER
void Switch_Int(void){
	
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
uint64_t OS_ISR_Count;

uint64_t OS_Clock_Period = 80;
int OS_Clock_Priority = 3; 
unsigned long OS_Clock_Time; 
int OS_Clock_Flag;
int OS_Clock_New;

uint32_t OS_Fifo[128];
int OS_Fifo_First;
int OS_Fifo_Last;
int OS_Fifo_Length;
unsigned long Mailbox; 


/********* OS_Suspend **************
Stop execution of currently active
foreground thread. Move on to next.
************************************/
void OS_Suspend(void){  // TODO: Deal with sleep
	NVIC_ST_CURRENT_R = 0;  // clear counter
	NVIC_INT_CTRL_R = 0x04000000;  // trigger systick
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, uint16_t value){
	semaPt->Value = value;  // value = number of threads
	// that can access resource at one time
}

/********** OS_ISR *************
what to run on systick interrupt
********************************/
void OS_ISR(void);

void OS_Clock_ISR(void) {
	OS_Clock_Time++;
}

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
	SysTick_Init(OS_ISR_period, OS_ISR_priority);	
	//Switch_Init(&Switch_Int, 1);
	//timer_init_fns[0] = &Timer0A_Init; // Also hacky since already initing in ADCTrigger
	timer_init_fns[1] = &Timer1A_Init;
	timer_init_fns[2] = &Timer2A_Init;
	timer_init_fns[3] = &Timer3A_Init;
	
	// Periodic Clock Task
	OS_Clock_Time = 0;
	timer_init_fns[1](&OS_Clock_ISR, OS_Clock_Period, OS_Clock_Priority);
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
	for (int16_t i = 2; i < sizeof(timer_occupied); i++){ // Hacking to free up two tim ers for our own usage
		if(!timer_occupied[i]){
			timer_to_use = i;
			break;
		}
	}
	if (timer_to_use == -1){ return false; }  // no free timers
	timer_occupied[timer_to_use] = true;
	timer_init_fns[timer_to_use](task, period, priority);  // TODO: set priority too
	return true;						
}


//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), int priority) {
	Switch_Init(task, priority);
	return 1; // NOT FULLY IMPLEMENTED (does not return 0 if failure)
}


//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority){
	//IGNORED
	return 0;
}

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size) {
	OS_Fifo_Length = size;
	OS_Fifo_First = 0;
	OS_Fifo_Last = 0;
	OS_InitSemaphore(&Mutex, 1);
	OS_InitSemaphore(&DataAvailable, 0);
	
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data) {
// from lecture 5 slides, redo fifo
//	if get or put called in background	// 
	//OS_Wait(&DataRoomLeft)
	OS_bWait(&Mutex);
	OS_Fifo[OS_Fifo_Last] = data;
	OS_Fifo_Last = (OS_Fifo_Last + 1) % OS_Fifo_Length;
	if(OS_Fifo_Last == OS_Fifo_First) { // OVERWRITE OCCURED
		OS_Fifo_First = OS_Fifo_Last;
		OS_bSignal(&Mutex);
		return 0;
	}
	OS_bSignal(&Mutex);
	OS_Signal(&DataAvailable);
	return 1;
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void) {
// from lecture 5 slides, redo fifo
// if get or put called in background
	OS_Wait(&DataAvailable);
	OS_bWait(&Mutex);
	unsigned long data = OS_Fifo[OS_Fifo_First];
	OS_Fifo_First = (OS_Fifo_First + 1) % OS_Fifo_Length;
	OS_bSignal(&Mutex);
	// OS_Signal(&DataRoomLeft)
	return data;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void) {
	if(OS_Fifo_Last != OS_Fifo_First) {
		if(OS_Fifo_Last > OS_Fifo_First) {
			return (long)(OS_Fifo_Last - OS_Fifo_First);
		} else {
			return (long)(OS_Fifo_Last + OS_Fifo_Length - OS_Fifo_First - 1);
		}
	}
	return 0;
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void) {
	OS_InitSemaphore(&MailboxFull, 0);
	OS_InitSemaphore(&MailboxEmpty, 1);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data) {
// from lecture 5 slides, redo mailbox if
// send called in background
	OS_bWait(&MailboxEmpty);
	Mailbox = data;
	OS_bSignal(&MailboxFull);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void) {
// from lecture 5 slides
	OS_bWait(&MailboxFull);
	unsigned long data = Mailbox;
	OS_bSignal(&MailboxEmpty);
	return data;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void) {
	unsigned long result = OS_Clock_Time;
	result = result * 80;
	return result;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
	return (stop - start);// * 80; // If we get two times that were gotten from OS_Time then we shouldn't have to convert anything
}


/********** OS_Wait ************
WARNING: CANNOT BE CALLED WHEN 
INTERRUPTS ARE DISABLED!!!
*******************************/
void OS_Wait(Sema4Type *s){
	OS_DisableInterrupts();
	while(s->Value <= 0){
		OS_EnableInterrupts();
		OS_Suspend();  // other task runs here
		OS_DisableInterrupts();
	}
	// see lecture 5 for
	// blocking implementation pseudocode
	s->Value = s->Value - 1;  // mark resource allocated
	OS_EnableInterrupts();
}
//******* OS_Signal***********
//****************************
void OS_Signal(Sema4Type *s){
	// see lecture 5 for
	// blocking implementation pseudocode
	long status;
	status = StartCritical();
	s->Value = s->Value + 1;  // free resource
	EndCritical(status);
}

/******** OS_bWait ************
 Lab2 spinlock, set to 0
 Lab3 block if less than zero
 input:  pointer to a binary semaphore
 output: none
 WARNING: CANNOT BE CALLED WHEN 
INTERRUPTS ARE DISABLED!!! 
*******************************/
void OS_bWait(Sema4Type *semaPt){
	OS_DisableInterrupts();
	while(!semaPt->Value){
		OS_EnableInterrupts();
		OS_Suspend();  // other task runs here
		OS_DisableInterrupts();
	}
	semaPt->Value = 0;
	OS_EnableInterrupts();	
}
// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	long status;
	status = StartCritical();
	semaPt->Value = 1;  // free resource
	EndCritical(status);
}	

#endif
