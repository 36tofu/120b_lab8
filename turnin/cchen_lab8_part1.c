/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void set_PWM(double frequency){
	static double current_frequency;

	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		else {TCCR3B |= 0x03; }

		if (frequency < 0.954) { OCR3A = 0xFFFF; }

		else if (frequency > 31250) { OCR3A = 0x0000; }

		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT3 = 0;
		current_frequency = frequency; 
	}
}

void PWM_on(){
	TCCR3A = (1 << COM3A0);

	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

	set_PWM(0);
}

void PWM_off(){
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

unsigned long _avr_timer_M = 1; //start count from here, down to 0. Dft 1ms
unsigned long _avr_timer_cntcurr = 0; //Current internal count of 1ms ticks


void TimerOn(){
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit 3 = 0: CTC mode (clear timer on compare)
	//AVR output compare register OCR1A
	OCR1A = 125; // Timer interrupt will be generated when TCNT1 == OCR1A
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; //bit1: OCIE1A -- enables compare match interrupt
	//Init avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR will be called every _avr_timer_cntcurr ms
	
	//Enable global interrupts 
	SREG |= 0x80; //0x80: 1000000

}

void TimerOff(){
	TCCR1B = 0x00; //bit3bit1bit0 = 000: timer off
}


ISR(TIMER1_COMPA_vect){
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


typedef struct task {
  int state; // Current state of the task
  unsigned long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[1];

const unsigned char tasksNum = 1;
const unsigned long tasksPeriodGCD = 1;
const unsigned long periodSample = 100;

enum FRQ_States { FRQ_SMStart, FRQ_s0, FRQ_1, FRQ_2, FRQ_3};
int TickFct_FRQ(int state);

void TimerISR() {
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}

int main() {

  DDRB = 0x40; 
  PORTB = 0x00;
  DDRA = 0x00;
  PORTA = 0x07;
  unsigned char i=0;
  tasks[i].state = FRQ_SMStart;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_FRQ;
  TimerSet(tasksPeriodGCD);

  TimerOn();
  PWM_on();
  
  while(1) {
     //Sleep();
  }
  return 0;
}


int TickFct_FRQ(int state) {
unsigned char tmpA = (~PINA & 0x07);
  switch(state) { // Transitions
     case FRQ_SMStart: // Initial transition
        state = FRQ_s0;
        break;
     case FRQ_s0:
	if(tmpA == 0x00)
		state = FRQ_s0;
	else if(tmpA == 0x01)
		state = FRQ_1;
	else if(tmpA == 0x02)
		state = FRQ_2;
	else if(tmpA == 0x04)
		state = FRQ_3;
	break;
     case FRQ_1:
	if(tmpA == 0x00)
		state = FRQ_s0;
	else if(tmpA == 0x01)
		state = FRQ_1;
	else
		state = FRQ_s0;
	break;
     case FRQ_2:
	if(tmpA == 0x00)
		state = FRQ_s0;
	else if(tmpA == 0x02)
		state = FRQ_2;
	else
		state = FRQ_s0;
	break;
     case FRQ_3:
	if(tmpA == 0x00)
		state = FRQ_s0;
	else if(tmpA == 0x04)
		state = FRQ_3;
	else
		state = FRQ_s0;
	break;
     default:
        state = FRQ_SMStart;
   } // Transitions

  switch(state) { // State actions
     case FRQ_s0:
	set_PWM(0);
        break;
     case FRQ_1:
	set_PWM(261.63);
        break;
     case FRQ_2:
	set_PWM(293.66);
        break;
     case FRQ_3:
	set_PWM(329.63);
        break;
     default:
        break;
  } // State actions
  return state;
}
