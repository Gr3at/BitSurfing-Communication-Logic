/* run this in kernel space to check how system timing works and which function may be better to use */

#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ktime.h>

long long int timeInNanosecs(void){
	long long int ohmy = ktime_to_ns(ktime_get());

	return(ohmy>>3);
}

static int __init hello_start(void){
	long long int timenanosec = timeInNanosecs();
	//long long int timeusec = Microsecs();
	//ktime_t ktime_sub(ktime_t kt1, ktime_t kt2);  /* kt1 - kt2 */
	//u64 ktime_to_ns(ktime_t kt);
	//s64 ktime_to_us(const ktime_t kt);
	//s64 ktime_to_ms(const ktime_t kt);

	printk(KERN_INFO "Loading hello module...\n"); 
	printk(KERN_INFO "k time : %lld nanoseconds\n", timenanosec);
	//printk(KERN_INFO "do gettimeofday : %lld useconds\n", timeusec);
	printk(KERN_INFO "k time : %lld mseconds\n", ktime_to_ms(ktime_get()));
	printk(KERN_INFO "k time : %lld useconds\n", ktime_to_us(ktime_get()));
	return(0); 
}
  
static void __exit hello_end(void){
	printk(KERN_INFO "Goodbye Mr.\n"); 
}
  
module_init(hello_start); 
module_exit(hello_end);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("me");
MODULE_DESCRIPTION("Total Garbage");
MODULE_VERSION("1");

/* 
obj-m = hello.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
*/

