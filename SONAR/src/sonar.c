/*
 * sonar.c
 *
 *  Created on: Feb 22, 2017
 *      Author: ikhatckevich
 */

#include "sonar.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define B_SONAR_INPROGRESS (0)
#define B_SONAR_OBJ_PRESENT (1)
#define B_SONAR_OBJ_PR_CHGD (2)

#define F_SONAR_INPROGRESS (1<<B_SONAR_INPROGRESS)
#define F_SONAR_OBJ_PRESENT (1<<B_SONAR_OBJ_PRESENT)
#define F_SONAR_OBJ_PR_CHGD (1<<B_SONAR_OBJ_PR_CHGD)

volatile uint8_t sonar_flags = 0;
volatile uint16_t sonar_result;

void sonar_init(void){
	/* ports:
	 * trigger: PD7, out
	 * echo: PB0 (ICP1), in
	 */
	DDRD |= 1<<PD7;

	//configure sample timer0
	TIMSK |= 1<<TOIE0; //enable ovf interrupt
	TCCR0 |= (1<<CS02 | 1<<CS00); //clk/1024. ovf occurred every 32ms (for 8Mhz CPU-clk)

	//configure timer1
	//TCCR1B |= (1<CS11); //clk/1. ovf occured every 65.6ms (for 8MHz CPU-clk)
	TIMSK |= 1<<TOIE1; //enable overflow int.
	sei();
}

/*sample timer interrupt*/
ISR(TIMER0_OVF_vect){
	//process wher mesuring not inprogress and echo in LOW
	if (!(sonar_flags & F_SONAR_INPROGRESS) && !(PINB & (1<<PB0))){
		sonar_flags |= F_SONAR_INPROGRESS; //set "busy" flag

		/*configure capturing echo on rising edge*/
		TIMSK &= ~(1<<TICIE1); //disable interrupt
		TCCR1B |= (1<<ICES1); //set capture edge
		TIFR |= (1<<ICF1); //clear interrupt flag, generated on inverting edge
		TIMSK |= (1<<TICIE1); //enable interrupt

		PORTD |= 1<<PD7; //request sample
	}
}

/*echo capture interrupt*/
ISR(TIMER1_CAPT_vect){
	//PORTD |= (1<<PD6);
	if (sonar_flags & F_SONAR_INPROGRESS){
		if (TCCR1B & (1<<ICES1)){
			//rising edge captured
			TCCR1B |= (1<<CS10); //start counting (from 0)

			PORTD &= ~(1<<PD7); //reset sample request

			TIMSK &= ~(1<<TICIE1); //disable interrupt
			TCCR1B ^= (1<<ICES1); //invert capture edge
			TIFR |= (1<<ICF1); //clear interrupt flag, generated on inverting edge
			TIMSK |= (1<<TICIE1); //enable interrupt

		}else{
			sonar_result = ICR1; //save result
			//T1 stop, reset, reset prescaler
			TCCR1B &= ~(1<<CS12 | 1<<CS11 | 1<<CS10);
			TCNT1 = 0x0000;
			//SFIOR |= PSR10; //WARNING! t0 and t1 use the same prescaler.
			//falling edge captured (result is ready)
			//set "change presence" flag
			if (sonar_flags & F_SONAR_OBJ_PRESENT){
				sonar_flags &= ~F_SONAR_OBJ_PR_CHGD;
			}else{
				sonar_flags |= F_SONAR_OBJ_PR_CHGD;
				sonar_presence_changed(1);
			}

			sonar_flags |= F_SONAR_OBJ_PRESENT; //flag indicates that object is present
			sonar_flags &= ~F_SONAR_INPROGRESS; //measured OK
		}
	}
}

ISR(TIMER1_OVF_vect){
	//interrupt that occurred when object not in range or too far
	if (sonar_flags & F_SONAR_INPROGRESS){
		//set "change presence" flag
		if (sonar_flags & F_SONAR_OBJ_PRESENT){
			sonar_flags |= (F_SONAR_OBJ_PR_CHGD);
			sonar_presence_changed(0);
		}else{
			sonar_flags &= ~F_SONAR_OBJ_PR_CHGD;
		}

		sonar_flags &= ~F_SONAR_OBJ_PRESENT; //object not in range
		sonar_flags &= ~F_SONAR_INPROGRESS; //reset busy flag
	}
}
