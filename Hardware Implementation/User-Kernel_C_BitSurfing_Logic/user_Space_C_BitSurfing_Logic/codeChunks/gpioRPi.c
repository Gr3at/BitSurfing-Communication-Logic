#include <stdio.h>
#include <wiringPi.h>

#define INPIN 17
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

/////////////////////////////////////////////////////////
/* very informative post : http://vabi-robotics.blogspot.com/2013/10/register-access-to-gpios-of-beaglebone.html */
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ELAPSEDTIME ((double)(clock()-t))/CLOCKS_PER_SEC
#define GPIO2_BASE_ADDR (0x20000000 + 0x200000)
#define GPIO2BANK_SIZEINBYTES 4096
#define INP_GPIO(g) *(map_base+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(map_base+((g)/10)) |=  (1<<(((g)%10)*3))
#define GET_GPIO(g) (*(map_base+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH
#define GPIO_SET *(map_base+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(map_base+10) // clears bits which are 1 ignores bits which are 0

#define INPIN 17 // board pin 11
#define OUTPIN 27 // board pin 13

//0xFFFFFFFF ^ (1 << GPIO2_2_RELATIVE_POSITION) = fffffffb
//11111111111111111111111111111111 ^ 00000000000000000000000000000100 = 11111111111111111111111111111011

int main(void){
    clock_t t = clock();
    //int err = system("config-pin P8.7 hi");
    //printf("%d\n",err);
    //err = system("config-pin P8.9 in");

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    unsigned long *map_base = (unsigned long*) mmap(NULL, GPIO2BANK_SIZEINBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO2_BASE_ADDR);
    
    INP_GPIO(INPIN);
    INP_GPIO(OUTPIN); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(OUTPIN);
    
    while(1){
        GPIO_SET = 1<<OUTPIN;  //GPIO.output(outputChannelPin, GPIO.HIGH)
        sleep(5);
        GPIO_CLR = 1<<OUTPIN; //GPIO.output(outputChannelPin, GPIO.LOW)
        printf("data now : %lu\n",map_base[GPIO_DATAIN/4]);
        if(GET_GPIO(INPIN))  //if(GPIO.input(inputChannelPin)==GPIO.HIGH)
            printf("input high\n");
    }
    printf("fun() took %f seconds to execute \n", ELAPSEDTIME);
    close(fd);
    return(0);
}

//////////////////////////////////////////////////////////
