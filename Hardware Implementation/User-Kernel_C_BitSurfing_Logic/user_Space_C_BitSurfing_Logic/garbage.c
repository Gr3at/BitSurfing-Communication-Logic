#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

void *fun1(void *);
void *fun2(void *);

int main(){
	pthread_t id1,id2;
	pthread_create (&id1,NULL,&fun1,NULL);
	pthread_create (&id2,NULL,&fun2,NULL);
	pthread_join(id1,NULL);
	pthread_join(id2,NULL);

	return 0;
}

void *fun1(void *p){
	for(int i=0;i<50;i++)
		printf ("Func 1 : %d \n",i);
}

void *fun2(void *p){
	for(int i=0;i<50	;i++)
		printf ("Func 2 : %d \n",i);
}

// piRasberrytemppwd : debianBBBtemppwd

/* RasPi 
   -----
   Input : 11
   Output: 13 (initial level:low)

   cat /sys/class/gpio/gpio17/value
e.g. We want to wait for a falling-edge interrupt on GPIO pin 0, so to setup the hardware, we need to run:
   gpio edge 0 falling
before running the program.

or 
int wiringPiISR(int pin, int edgeType, void (*function)(void));


   BBB
   ---
   Input : 9(GPIO_69)(GPIO2_5) (formula is Mx32+Y --> 2x32+5 = 69)
   Output: 7(GPIO_66)(GPIO2_2) (formula is Mx32+Y --> 2x32+2 = 66)(initial level:low)

   Terminal Use : 
   echo 66 > /sys/class/gpio/export
   echo out > /sys/class/gpio/gpio66/direction
   echo 0 > /sys/class/gpio/gpio66/value

   echo 66 > /sys/class/gpio/unexport


   ----------------------------------------------------------------------------------
   	config-pin -h
   	config-pin P8.7 out // setup pin P8_7 to GPIO output
   	config-pin P8.7 hi // setup pin P8_7 to GPIO output and assign value of 1
   	config-pin P8.7 lo // setup pin P8_7 to GPIO output and assign value of 0
   	config-pin P8.9 in // setup pin P8_9 to GPIO input
   	config-pin P8.9 in_pd // setup pin P8_9 to GPIO input and associate pull-down resistor
*/