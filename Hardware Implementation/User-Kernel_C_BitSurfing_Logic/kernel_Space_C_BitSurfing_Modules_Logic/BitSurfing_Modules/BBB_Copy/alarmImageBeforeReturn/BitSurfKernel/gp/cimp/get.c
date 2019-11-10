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
    volatile unsigned long *map_base = (volatile unsigned long*) mmap(NULL, GPIO2BANK_SIZEINBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO2_BASE_ADDR);
printf("11\n");
map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << GPIO2_2_RELATIVE_POSITION)); //GPIO.setup(outputChannelPin, GPIO.OUT)
printf("15\n");
    map_base[OE_ADDR/4] |= (1 << GPIO2_5_RELATIVE_POSITION);    //GPIO.setup(inputChannelPin, GPIO.IN)
    printf("1\n");
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
