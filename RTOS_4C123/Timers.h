/*#include <stdbool.h>
#include "../inc/tm4c123gh6pm.h"
#include <assert.h>

#define NUMTIMERS 6

struct timer{
	uint16_t activate;  // mask to turn on
	uint16_t enable;    // 
	uint16_t mode;  // number of bits for timer
	uint16_t tamr;  // periodic mode
	uint16_t tail;  // reload value (period)
	uint16_t bus;   // bus clock resolution
	uint16_t flag;  // timeout flag
	uint16_t interrupt; // timer interrupt
	uint16_t priority_addr;
	uint16_t priority_shift;
	void(*task) (void);  // process for timer
};
typedef struct timer timerType;
timerType timers[NUMTIMERS];

int nvic_en_shift_base = 19;
void define_timers(){
	timers[0].activate = 0x01;
	timers[0].enable = TIMER0_CTL_R;
	timers[0].mode = TIMER0_CFG_R;
	timers[0].tamr = TIMER0_TAMR_R;
	timers[0].tail = TIMER0_TAILR_R;
	timers[0].bus = TIMER0_TAPR_R;
	timers[0].flag = TIMER0_ICR_R; 
	timers[0].interrupt = TIMER0_IMR_R;
	timers[0].priority_addr = NVIC_PRI4_R;
	timers[0].priority_shift = 31;
	timers[1].activate = 0x01;
	timers[1].enable = TIMER0_CTL_R;
	timers[1].mode = TIMER0_CFG_R;
	timers[1].tamr = TIMER0_TBMR_R;
	timers[1].tail = TIMER0_TBILR_R;
	timers[1].bus = TIMER0_TBPR_R;
	timers[1].flag = TIMER0_ICR_R; 
	timers[1].interrupt = TIMER0_IMR_R;
	timers[1].priority_addr = NVIC_PRI5_R;
	timers[1].priority_shift = 5;
	timers[2].activate = 0x02;
	timers[2].enable = ;
	timers[2].mode = ;
	timers[2].tamr = ;
	timers[2].tail = ;
	timers[2].bus = ;
	timers[2].flag ; 
	timers[2].interrupt = ;
	timers[2].priority = ;
	timers[2].nvic_en = ;
	timers[2].nvic_dis = ;
	timers[3].activate = ;
	timers[3].enable = ;
	timers[3].mode = ;
	timers[3].tamr = ;
	timers[3].tail = ;
	timers[3].bus = ;
	timers[3].flag ; 
	timers[3].interrupt = ;
	timers[3].priority = ;
	timers[3].nvic_en = ;
	timers[3].nvic_dis = ;
	timers[4].activate = ;
	timers[4].enable = ;
	timers[4].mode = ;
	timers[4].tamr = ;
	timers[4].tail = ;
	timers[4].bus = ;
	timers[4].flag ; 
	timers[4].interrupt = ;
	timers[4].priority = ;
	timers[4].nvic_en = ;
	timers[4].nvic_dis = ;
	timers[5].activate = ;
	timers[5].enable = ;
	timers[5].mode = ;
	timers[5].tamr = ;
	timers[5].tail = ;
	timers[5].bus = ;
	timers[5].flag ; 
	timers[5].interrupt = ;
	timers[5].priority = ;
	timers[5].nvic_en = ;
	timers[5].nvic_dis = ;
}

bool timer_occupied(uint16_t idx){ 
	return timers[idx].enable;
};

// period is number of clock cycles in PWM period ((1/clockfreq) units)
// high is number of clock cycles output is high ((1/clockfreq) units)
// duty cycle = high/period
// assumes that period>high>(approx 3)
void Timer_AddInterrupt(uint16_t timer_idx, 
											  void(*task) (void),
											  uint16_t period,
												uint16_t priority){
	assert(timer_idx > NUMTIMERS);
  SYSCTL_RCGCTIMER_R |= 0x01;      // activate timer0
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for alternate (PWM) mode
  TIMER0_TAMR_R = (TIMER_TAMR_TAAMS|TIMER_TAMR_TAMR_PERIOD);
  TIMER0_TAILR_R = period-1;       // timer start value
  TIMER0_TAMATCHR_R = period-high-1; // duty cycle = high/period
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, PWM
}
												
void Init_Timers(*/