/*
 * main.c
 *
 *  Created on: Feb 15, 2017
 *      Author: ikhatckevich
 */

#include "src/sonar.h"
#include <avr/io.h>
#include <util/delay.h>
#define F_OBJ_APPEARED (1)
#define F_OBJ_DISAPPEARED (2)

#define DEBUG_LED_ON	(PORTD |= (1<<PD6))
#define DEBUG_LED_OFF	(PORTD &= ~(1<<PD6))

volatile uint8_t flags = 0;

extern void sonar_presence_changed(uint8_t st){
	if (st){
		flags |= F_OBJ_APPEARED;
	}else{
		flags |= F_OBJ_DISAPPEARED;
	}
}

void init(void){
	DDRD |= (1<<PD6); //debug led
	DEBUG_LED_ON;
	_delay_ms(1000);
	DEBUG_LED_OFF;
	_delay_ms(1000);
	DEBUG_LED_ON;
	_delay_ms(1000);
	DEBUG_LED_OFF;
	_delay_ms(1000);
	sonar_init();
}

int main(void){
	init();

	while(1){
		if (flags & F_OBJ_APPEARED){
			flags &= ~F_OBJ_APPEARED;
			DEBUG_LED_ON;
		}
		if (flags & F_OBJ_DISAPPEARED){
			flags &= ~F_OBJ_DISAPPEARED;
			DEBUG_LED_OFF;
		}
	}
	return 0;
}
