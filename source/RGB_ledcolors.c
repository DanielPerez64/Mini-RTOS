/*
 * RGB_ledcolors.c
 *
 *  Created on: 7 may. 2024
 *      Author: Navy-PC
 */


#include <stdbool.h>

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_port.h"

#include "MK64F12.h"
#include "Bits.h"

void Off_led (){
	GPIO_PortSet(GPIOE,TRUE << bit_26); //Verde OF
	GPIO_PortSet(GPIOB,TRUE << bit_21); //Azul OFF
	GPIO_PortSet(GPIOB,TRUE << bit_22); //Rojo OFF
}

void Rojo (){
	Off_led ();
	GPIO_PortClear(GPIOB,TRUE << bit_22); //Rojo ON
}

void Verde (){
	Off_led ();
	GPIO_PortClear(GPIOE,TRUE << bit_26); //Verde ON
}

void Azul (){
	Off_led ();
	GPIO_PortClear(GPIOB,TRUE << bit_21); //Azul ON
}

void Morado (){
	Off_led ();
	GPIO_PortClear(GPIOB,TRUE << bit_21); //Azul ON
	GPIO_PortClear(GPIOB,TRUE << bit_22); //Rojo ON
}

void Amarillo (){
	Off_led ();
	GPIO_PortClear(GPIOE,TRUE << bit_26); //Verde ON
	GPIO_PortClear(GPIOB,TRUE << bit_22); //Rojo ON
}

void Cyan (){
	Off_led ();
	GPIO_PortClear(GPIOB,TRUE << bit_21); //Azul ON
	GPIO_PortClear(GPIOE,TRUE << bit_26); //Verde ON
}

void Blanco (){
	Off_led ();
	GPIO_PortClear(GPIOE,TRUE << bit_26); //Verde ON
	GPIO_PortClear(GPIOB,TRUE << bit_22); //Rojo ON
	GPIO_PortClear(GPIOB,TRUE << bit_21); //Azul ON
}


