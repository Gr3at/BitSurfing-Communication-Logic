#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/gpio.h>

/* Valid only for BeagleBoneBlack with AM335x ARM Cortex-A8 Microprocessor */
#define GPIO2_BASE_ADDR 0x481AC000
#define GPIO2BANK_SIZEINBYTES 4096
#define OE_ADDR 0x134
#define GPIO_DATAOUT 0x13C
#define GPIO_DATAIN 0x138
#define OUTPIN 66 // board pin P8.7 it may need 66 https://github.com/derekmolloy/boneDeviceTree/blob/master/docs/BeagleboneBlackP8HeaderTable.pdf
#define INPIN 69 // board pin P8.9 it may need 69
#define GET_GPIO(g) (map_base[GPIO_DATAIN/4] & (1 << g)) // 0 if LOW, (1<<g) if HIGH
/* ----------------------------------------------------------------------- */

volatile unsigned long *map_base;
static int state;

static void flash_timer_callback(void){
	if (0 == state){
		printk ("tick\n");
		map_base[GPIO_DATAOUT/4]  |= (1 << OUTPIN);
		state = 1;
	}
	else{
		printk ("tock\n");
		map_base[GPIO_DATAOUT/4]  ^= (1 << OUTPIN);
		state = 0;
	}
}

static int init_flash(void){
	printk ("Hello\n");
	state = 0;

	map_base = (volatile unsigned long*)ioremap(GPIO2_BASE_ADDR, GPIO2BANK_SIZEINBYTES);
  	printk ("Hello0\n");
  	if(((void*)map_base) == NULL){
    	printk( KERN_ERR "ioremap failed.\n Are you root?\n");
    	return(-1);
  	}
  	
  	if(!gpio_is_valid(INPIN) || !gpio_is_valid(OUTPIN) || gpio_request(INPIN, "INPIN") || gpio_request(OUTPIN, "OUTPIN")){
  		printk(KERN_CRIT "GPIO initialization failed.\n");
        return -EINVAL;
  	}
  	printk ("Hello2\n");
  	map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << OUTPIN)); // setup output pin
  	map_base[OE_ADDR/4] |= (1 << INPIN);    // setup input pin
  	printk ("Hello3\n");
 	int i;
 	for(i=0; i<10; i++){
 		flash_timer_callback();
 		if(GET_GPIO(INPIN)){
 			printk("Got something...\n");
 			flash_timer_callback();
 		}
 	}

 	return 0;
}

static void cleanup_flash(void){
 printk ("Goodbye\n");
 if(map_base){
    iounmap((void*)map_base);
  }
  gpio_free(INPIN);
  gpio_free(OUTPIN);
}

module_init(init_flash);
module_exit(cleanup_flash);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lap");
MODULE_DESCRIPTION("gpio stuff");