// OUTPIN : board pin 11 (apo ta 40) i alios gpio pin 17 i alios con1-P11 - PC4 - i alios apo wiringPi : BPI_M3_11 GPIO_PC04 = 4 + GPIO_PC00 (64) = 68 // check bpi-gpio.h for more pins
// INPIN (an xreiastei) : board pin 13 (apo ta 40) i alios gpio pin 27 i alios con1-P13 - PC7 i allios : BPI_M3_13 GPIO_PC07 = 7 + GPIO_PC00 (64) = 71

/* 
gpio = (bank * 32) + pin
gpioPC4 = 2 * 32 + 4 = 68
gpioPC7 = 2 * 32 + 7 = 71

echo 68 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio68/direction
cat /sys/class/gpio/gpio68/value
echo 1 > /sys/class/gpio/gpio68/value
echo 0 > /sys/class/gpio/gpio68/value
echo 68 > /sys/class/gpio/unexport

*/

#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define CCU_MODULE_ADDRESS 0x01c20000 /* need this for mmap (since we have to use a value multiple of page size, i.e. 4096) */
#define BLOCK_SIZE 4096
#define GPIO_BASE_BP_RELATIVE_ADDR 0x800
#define PC_CFG0_REG_RELATIVE_ADDR 0x48 /* Port C GPIO address! The code below is valid only for this port (PC) of BPi - M3 */
#define PC_DATA_REG_RELATIVE_ADDR 0x10
#define INPIN 4 /* PC4 or board pin 11 or gpio 68 or bcm 17 */
#define OUTPIN 7 /* PC7 or board pin 13 or gpio 71 or bcm 27 */
#define TURN_OFF_PC_PUD_0_15(pin) pc_gpio[0x1c/4] &= (0xFFFFFFFF ^ (1 << 2*pin)) /* 0x1c = 0x64 - 0x48 which is the relative distance from pc_gpio address -# default value is 0x5140 = 0b101000101000000 */
#define SET_PC_INP_GPIO(pin) *(pc_gpio) &= (0xFFFFFFFF ^ (7 << 4*pin)) /* default value 0x77777777 = 0b0 111 011101110 111 0111011101110111 */
#define SET_PC_OUT_GPIO(pin) *(pc_gpio) |= (1 << 4*pin) /* default value 0x77777777 = 0b0 111 011101110 111 0111011101110111 */
#define READ_PC_PIN_VALUE(pin) (pc_gpio[PC_DATA_REG_RELATIVE_ADDR/4]&(1<<pin)) /* 0x10 = 0x58 - 0x48 -# returns 0 if LOW, (1<<g) if HIGH */

/* Remark : since pc_gpio is of type volatile unsigned long (4 Bytes = 32 bits) we have to divide all relative addresses by 4 */
static volatile unsigned long *pc_gpio;

int main(){
	int fd;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC) ) < 0){
		printf("can't open dev/mem\n");
    	return(1);
	}

    pc_gpio = (volatile unsigned long *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)CCU_MODULE_ADDRESS);
    if((void *)pc_gpio==MAP_FAILED){
    	printf("mmap Failed\n");
    	return(1);
    }
    close(fd);
    printf("before :%p\n", pc_gpio);
    pc_gpio = &pc_gpio[(GPIO_BASE_BP_RELATIVE_ADDR+PC_CFG0_REG_RELATIVE_ADDR)/4];
    printf("after pc : %p\n", pc_gpio);

    return(1);
    /* turn off pull up down */
    TURN_OFF_PC_PUD_0_15(INPIN);
    TURN_OFF_PC_PUD_0_15(OUTPIN);
    //printf("PUD after : %ld\n", pc_gpio[0x1c/4]);
    
    /* set pin modes */
    //printf("PORTC PINS before : %ld\n", *(pc_gpio));
    SET_PC_INP_GPIO(INPIN);
    SET_PC_INP_GPIO(OUTPIN);	// must use SET_PC_INP_GPIO() before SET_PC_OUT_GPIO()
    SET_PC_OUT_GPIO(OUTPIN);
    //printf("PORTC PINS after : %ld\n", *(pc_gpio));
    
    printf("Data : %ld\n", pc_gpio[PC_DATA_REG_RELATIVE_ADDR/4]);
    printf("inpin : %ld\n",READ_PC_PIN_VALUE(INPIN));
    //printf("outpin : %d\n",READ_PC_PIN_VALUE(OUTPIN));
    pc_gpio[PC_DATA_REG_RELATIVE_ADDR/4] |= (1<<OUTPIN); // set output to high
    printf("Data : %ld\n", pc_gpio[PC_DATA_REG_RELATIVE_ADDR/4]);
    printf("inpin : %ld\n",READ_PC_PIN_VALUE(INPIN));
    pc_gpio[PC_DATA_REG_RELATIVE_ADDR/4] ^= (1<<OUTPIN); // set output to low

	return(0);
}