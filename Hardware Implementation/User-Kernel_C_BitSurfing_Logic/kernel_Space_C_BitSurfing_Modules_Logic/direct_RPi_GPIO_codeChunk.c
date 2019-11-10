#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>

/* Valid only for Raspberry Pi with BCM2835 ARM Microprocessor */
#define GPIO_BASE (0x20000000 + 0x200000)
#define BLOCK_SIZE 4096
#define INP_GPIO(g) *(map_base+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(map_base+((g)/10)) |=  (1<<(((g)%10)*3))
#define GET_GPIO(g) (*(map_base+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH
//#define GPIO_READ(g) *(gpio.addr + 13) &= (1<<(g))
#define GPIO_SET *(map_base+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(map_base+10) // clears bits which are 1 ignores bits which are 0

#define INPIN 17 // board pin 11
#define OUTPIN 27 // board pin 13
/* ----------------------------------------------------------------------- */

static volatile unsigned long *map_base;
static int state;

static void flash_timer_callback(void){
	if (0 == state){
		printk ("tick\n");
		GPIO_SET = 1<<OUTPIN;
		state = 1;
	}
	else{
		printk ("tock\n");
		GPIO_CLR = 1<<OUTPIN;
		state = 0;
	}
}

static int init_flash(void){
	printk ("Hello\n");
	state = 0;

	map_base = (static volatile unsigned long *)ioremap(GPIO_BASE, BLOCK_SIZE);
	INP_GPIO (INPIN);
	INP_GPIO (OUTPIN);
	OUT_GPIO (OUTPIN);
 	
 	int i;
 	for(i=0; i<10000000; i++){
 		if(GET_GPIO(INPIN)){
 			printk("Got something...\n");
 			flash_timer_callback();
 		}
 	}

 	return 0;
}

static void cleanup_flash(void){
 printk ("Goodbye\n");
 GPIO_CLR = 1<<OUTPIN;
 if(map_base){
    iounmap((void*)map_base);
  }
}

module_init(init_flash);
module_exit(cleanup_flash);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lap");
MODULE_DESCRIPTION("gpio stuff");