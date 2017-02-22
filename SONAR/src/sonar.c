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


}

/*sample timer interrupt*/
ISR (TIMER0_OVF_vect){
	if (!(sonar_flags & F_SONAR_INPROGRESS)){
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
	if (TCCR1B & (1<<ICES1)){
		//rising edge captured
		TCNT1 = 0x0000; //reset timer
		PORTD &= ~(1<<PD7); //reset sample request

		TIMSK &= ~(1<<TICIE1); //disable interrupt
		TCCR1B ^= (1<<ICES1); //invert capture edge
		TIFR |= (1<<ICF1); //clear interrupt flag, generated on inverting edge
		TIMSK |= (1<<TICIE1); //enable interrupt

	}else{
		//falling edge captured (result is ready)
		sonar_result = ICR1; //save result
		//set "change presence" flag
		if (sonar_flags & F_SONAR_OBJ_PRESENT){
			sonar_flags &= ~F_SONAR_OBJ_PR_CHGD;
		}else{
			sonar_flags |= F_SONAR_OBJ_PR_CHGD;
		}

		sonar_flags |= F_SONAR_OBJ_PRESENT; //flag indicates that object is present
		sonar_flags &= ~F_SONAR_INPROGRESS; //measured OK
	}
}

ISR(TIMER1_OVF_vect){
	//interrupt that occurred when object not in range or too far
	if (sonar_flags & F_SONAR_INPROGRESS){
		//set "change presence" flag
		if (sonar_flags & F_SONAR_OBJ_PRESENT){
			sonar_flags |= (F_SONAR_OBJ_PR_CHGD);
		}else{
			sonar_flags &= ~F_SONAR_OBJ_PR_CHGD;
		}

		sonar_flags &= ~F_SONAR_OBJ_PRESENT; //object not in range
	}
}
