#ifndef __BOARD_H__
#define __BOARD_H__

//include standard headers for SAM9260
#include "include/AT91SAM9260.h"		//from Atmel
#include "include/lib_AT91SAM9260.h" 	//unofficial one (only few functions)

//global defs
#define EXT_OC          18432000  // External Oscillator frequency
#define MCK             (99328000)  // MCK, master clock for peripherals
#define PPLACK          198656000  // Clock for ARM core

//other switches
#define SW1_PIO			AT91C_BASE_PIOC
#define SW1_MASK		AT91C_PIO_PC15
#define SW2_PIO			AT91C_BASE_PIOC
#define SW2_MASK		AT91C_PIO_PC14
#define SW3_PIO			AT91C_BASE_PIOC
#define SW3_MASK		AT91C_PIO_PC13
#define SW4_PIO			AT91C_BASE_PIOA
#define SW4_MASK		AT91C_PIO_PA7

//other LEDS
#define LED_R_PIO		AT91C_BASE_PIOB
#define LED_R_MASK		AT91C_PIO_PB8
#define LED_G_PIO		AT91C_BASE_PIOA
#define LED_G_MASK		AT91C_PIO_PA6
#define LED_Y_PIO		AT91C_BASE_PIOA
#define LED_Y_MASK		AT91C_PIO_PA9
#define LED_B_PIO		AT91C_BASE_PIOB
#define LED_B_MASK		AT91C_PIO_PB9

//interface defs -- to be used in the program

#define SW1_PRESSED		( ( SW1_PIO->PIO_PDSR & SW1_MASK ) == 0)
#define SW2_PRESSED		( ( SW2_PIO->PIO_PDSR & SW2_MASK ) == 0)
#define SW3_PRESSED		( ( SW3_PIO->PIO_PDSR & SW3_MASK ) == 0)
#define SW4_PRESSED		( ( SW4_PIO->PIO_PDSR & SW4_MASK ) == 0)
#define LED_R_INIT		{LED_R_PIO->PIO_PER = LED_R_MASK; LED_R_PIO->PIO_OER = LED_R_MASK; LED_R_PIO->PIO_OWER = LED_R_MASK;}
#define LED_R_ON		LED_R_PIO->PIO_SODR = LED_R_MASK
#define LED_R_OFF		LED_R_PIO->PIO_CODR = LED_R_MASK
#define LED_R_TOGGLE	LED_R_PIO->PIO_ODSR ^= LED_R_MASK
#define LED_Y_INIT		{LED_Y_PIO->PIO_PER = LED_Y_MASK; LED_Y_PIO->PIO_OER = LED_Y_MASK; LED_Y_PIO->PIO_OWER = LED_Y_MASK;}
#define LED_Y_ON		LED_Y_PIO->PIO_SODR = LED_Y_MASK
#define LED_Y_OFF		LED_Y_PIO->PIO_CODR = LED_Y_MASK
#define LED_Y_TOGGLE	LED_Y_PIO->PIO_ODSR ^= LED_Y_MASK
#define LED_B_INIT		{LED_B_PIO->PIO_PER = LED_B_MASK; LED_B_PIO->PIO_OER = LED_B_MASK; LED_B_PIO->PIO_OWER = LED_B_MASK;}
#define LED_B_ON		LED_B_PIO->PIO_SODR = LED_B_MASK
#define LED_B_OFF		LED_B_PIO->PIO_CODR = LED_B_MASK
#define LED_B_TOGGLE	LED_B_PIO->PIO_ODSR ^= LED_B_MASK
#define LED_G_INIT		{LED_G_PIO->PIO_PER = LED_G_MASK; LED_G_PIO->PIO_OER = LED_G_MASK; LED_G_PIO->PIO_OWER = LED_G_MASK;}
#define LED_G_ON		LED_G_PIO->PIO_SODR = LED_G_MASK
#define LED_G_OFF		LED_G_PIO->PIO_CODR = LED_G_MASK
#define LED_G_TOGGLE	LED_G_PIO->PIO_ODSR ^= LED_G_MASK
#define LEDS_INIT		{LED_R_INIT; LED_G_INIT; LED_Y_INIT; LED_B_INIT;}


#endif
