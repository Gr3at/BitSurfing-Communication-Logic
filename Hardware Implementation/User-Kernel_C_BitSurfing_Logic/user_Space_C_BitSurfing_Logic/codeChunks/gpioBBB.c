#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define ELAPSEDTIME ((double)(clock()-t))/CLOCKS_PER_SEC

void main(void){
    clock_t t = clock();
    int fd;
    fd = open("/sys/class/gpio/gpio66/value", O_WRONLY);
    write(fd, "1", 1);
    write(fd, "0", 1);
    printf("fun() took %f seconds to execute \n", ELAPSEDTIME);
}

/////////////////////////////////////////////////////////
/* very informative post : http://vabi-robotics.blogspot.com/2013/10/register-access-to-gpios-of-beaglebone.html */
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ELAPSEDTIME ((double)(clock()-t))/CLOCKS_PER_SEC
#define GPIO2_BASE_ADDR 0x481AC000
#define GPIO2BANK_SIZEINBYTES 4096
#define OE_ADDR 0x134
#define GPIO_DATAOUT 0x13C
#define GPIO_DATAIN 0x138
#define GPIO2_2_RELATIVE_POSITION 2 //P8.7
#define GPIO2_5_RELATIVE_POSITION 5 //P8.9

//0xFFFFFFFF ^ (1 << GPIO2_2_RELATIVE_POSITION) = fffffffb
//11111111111111111111111111111111 ^ 00000000000000000000000000000100 = 11111111111111111111111111111011

int main(void){
    clock_t t = clock();
    //int err = system("config-pin P8.7 hi");
    //printf("%d\n",err);
    //err = system("config-pin P8.9 in");

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    unsigned long *map_base = (unsigned long*) mmap(NULL, GPIO2BANK_SIZEINBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO2_BASE_ADDR);
    map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << GPIO2_2_RELATIVE_POSITION)); //GPIO.setup(outputChannelPin, GPIO.OUT)
    map_base[OE_ADDR/4] |= (1 << GPIO2_5_RELATIVE_POSITION);    //GPIO.setup(inputChannelPin, GPIO.IN)
    
    while(1){
        map_base[GPIO_DATAOUT/4]  |= (1 << GPIO2_2_RELATIVE_POSITION);  //GPIO.output(outputChannelPin, GPIO.HIGH)
        sleep(5);
        map_base[GPIO_DATAOUT/4]  ^= (1 << GPIO2_2_RELATIVE_POSITION); //GPIO.output(outputChannelPin, GPIO.LOW)
        printf("data now : %lu\n",map_base[GPIO_DATAIN/4]);
        if(map_base[GPIO_DATAIN/4] & (1 << GPIO2_5_RELATIVE_POSITION))  //if(GPIO.input(inputChannelPin)==GPIO.HIGH)
            printf("input high\n");
    }
    printf("fun() took %f seconds to execute \n", ELAPSEDTIME);
    close(fd);
    return(0);
}

//////////////////////////////////////////////////////////

/* 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ELAPSEDTIME ((double)(clock()-t))/CLOCKS_PER_SEC

int main(void){
    clock_t t = clock();
    int err = system("config-pin P8.7 hi");
    printf("%d\n",err);
    //popen();
    system("echo 0 > /sys/class/gpio/gpio66/value");
    system("echo 1 > /sys/class/gpio/gpio66/value");

    printf("fun() took %f seconds to execute \n", ELAPSEDTIME);
    
    return(0);
}
*/