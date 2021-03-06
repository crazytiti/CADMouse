
#include <avr/io.h>
#include "ppm.h"
#include "main.h"
#include <avr/interrupt.h>

#define PPM_TCNT TCNT1

#define CHANNELS (8)
volatile unsigned int ppm[CHANNELS];
volatile int chan=-1;

#define T_MIN (1*F_CPU/1000) // 1ms = 12000 Ticks @ 12Mhz = Stick min position
#define T_MAX (2*F_CPU/1000) // 2ms = 24000 Ticks @ 12Mhz = Stick max position
#define T_OUT (3*F_CPU/1000) // 3ms = 36000 Ticks @ 12Mhz = Timeout

volatile char ppmNewData;
volatile unsigned int timeOld;

/**
* Interrupt: Compare 1A match = timeout
*/
ISR(TIMER1_COMPA_vect){

  if (chan!=-1) {
    ppmNewData=1;
  }
  chan=-1;
}

/**
* Interrupt: ICP-Edge detect
*/
ISR(TIMER1_CAPT_vect){
  unsigned int time=ICR1;
  OCR1A=time+T_OUT;
  if (time>timeOld) {
    time=time-timeOld;
  } else {
    time=time+(0xffff-timeOld)+1;
  }
  timeOld=ICR1;
  if (chan>=0 && chan<CHANNELS) {
    ppm[chan]=time;
  }
  chan++;
}

/**
* Init Timer1
*/
void ppmInit() {

#ifndef TIMSK
  // for Mega48/88/168 (not tested with hardware!)
  #define TICIE1 ICIE1
  #define TIMSK TIMSK1
#endif

  // tested with ATMega8
  TCCR1B=(1<<CS10); // TClk=CPUClk (no prescaler)
  TCCR1B|=(1<<ICNC1); // enable input noise chanceler
  TCCR1B|=(1<<ICES1); // positive edge (comment out for inverted signals)
  //ACSR|=(1<<ACIC); // use analog comparator instead of icp-pin
  TIMSK=(1<<TICIE1)|(1<<OCIE1A); // ICP-Interrupt and Output Compare A Match enable

  ppmNewData=0;
  chan=-1;
}

/**
* Get PPM-Channel n (8-bit resolution)
*/
unsigned char ppmGet(int n) {
  unsigned int t=ppm[n];

  // conversion T_MIN...T_MAX -> 0...255
  if (t<T_MIN) t=T_MIN;
  t-=T_MIN;
  t=t/((T_MAX-T_MIN)/255);
  if (t>255) t=255;

  return ((unsigned char)t);
}

