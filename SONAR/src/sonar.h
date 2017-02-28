/*
 * sonar.h
 *
 *  Created on: Feb 15, 2017
 *      Author: ikhatckevich
 */
#include <stdint.h>

void sonar_init(void);
extern void sonar_presence_changed(uint8_t st);

