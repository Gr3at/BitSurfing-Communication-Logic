#include <stdio.h>
#include <wiringPi.h>

#define INPIN 27
#define OUTPIN 27

int main(void){
  //int inputValue;
  printf("Raspberry Pi blink\n");
  if(wiringPiSetupGpio() == -1)
    return 1;

  pinMode(OUTPIN, OUTPUT); // aka BCM_GPIO pin 27 or Physical pin 13 or WiringPi pin 2
  //pinMode(INPIN, INPUT); // aka BCM_GPIO pin 17 or Physical pin 11 or WiringPi pin 0
  //pullUpDnControl(INPIN, PUD_DOWN) ; // PUD_OFF or PUD_DOWN or PUD_UP

  //int wiringPiISR(int pin, int edgeType, void (*function)(void)); // INT_EDGE_FALLING, INT_EDGE_RISING, INT_EDGE_BOTH or INT_EDGE_SETUP


  for(;;){
    digitalWrite(OUTPIN, HIGH); // On
    delay(500);         // mS
    digitalWrite(OUTPIN, LOW); // Off
    delay(500);
    //inputValue = digitalRead(INPIN); 
  }
  
  return(0);
}
//gpio readall
//gcc -o blink blink.c -lwiringPi
//sudo ./blink
