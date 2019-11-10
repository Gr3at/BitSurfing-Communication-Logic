#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/fs.h>
#include "kfile.h"
#include "gpioStuff.h"

#include <linux/types.h> /* for bool and other types */
#include <asm/io.h>
#include <linux/gpio.h> /* for gpio_is_valid(), gpio_request() and gpio_free() */

#define PREFIX 0b1110
/* Valid only for BananaPi - M3 AND !!!!!!!ONLY for Port C (PC) registers!!!!!!!! with Allwinner A83T ARM Microprocessor */
#define CCU_MODULE_ADDRESS 0x01c20000 /* need this for mmap (since we have to use a value multiple of page size, i.e. 4096) */
#define GPIO_BLOCK_SIZE 4096
#define GPIO_BASE_BP_RELATIVE_ADDR 0x800
#define PC_CFG0_REG_RELATIVE_ADDR 0x48 /* Port C GPIO address! The code below is valid only for this port (PC) of BPi - M3 */
#define PC_DATA_REG_RELATIVE_ADDR 0x10
#define OUTPIN 4 /* PC4 or board pin 11 or gpio 68 or bcm 17 */
#define INPIN 7 /* PC7 or board pin 13 or gpio 71 or bcm 27 */
#define TURN_OFF_PC_PUD_0_15(pin) map_base[0x1c/4] &= (0xFFFFFFFF ^ (1 << 2*pin)) /* 0x1c = 0x64 - 0x48 which is the relative distance from map_base address -# default value is 0x5140 = 0b101000101000000 */
#define SET_PC_INP_GPIO(pin) *(map_base) &= (0xFFFFFFFF ^ (7 << 4*pin)) /* default value 0x77777777 = 0b0 111 011101110 111 0111011101110111 */
#define SET_PC_OUT_GPIO(pin) *(map_base) |= (1 << 4*pin) /* default value 0x77777777 = 0b0 111 011101110 111 0111011101110111 */
#define READ_PC_PIN_VALUE(pin) (map_base[PC_DATA_REG_RELATIVE_ADDR/4]&(1<<pin)) /* 0x10 = 0x58 - 0x48 -# returns 0 if LOW, (1<<pin) if HIGH */

#define INPINSYSFS 71 /* board pin 13 --> 71 */
#define OUTPINSYSFS 68 /* board pin 11 --> 68 */
/* ground is board pin 6 */
/* ----------------------------------------------------------------------- */

/* Remark : since map_base is of type volatile unsigned long (4 Bytes = 32 bits) we have to divide all relative addresses by 4 */
static volatile unsigned long *map_base = NULL;

int init_gpio(void){
  map_base = (volatile unsigned long*)ioremap(CCU_MODULE_ADDRESS, GPIO_BLOCK_SIZE);
  map_base = &map_base[(GPIO_BASE_BP_RELATIVE_ADDR+PC_CFG0_REG_RELATIVE_ADDR)/4];
  if(((void*)map_base) == NULL){
    printk( KERN_ERR "ioremap failed.\n Are you root?\n");
    return(-1);
  }
  /*
  if(!gpio_is_valid(INPINSYSFS) || !gpio_is_valid(OUTPINSYSFS) || gpio_request(INPINSYSFS, "INPIN") || gpio_request(OUTPINSYSFS, "OUTPIN")){
    printk(KERN_CRIT "GPIO initialization failed.\n"); // /sys/class/gpio/export
    return(-1);
  }
  */
  TURN_OFF_PC_PUD_0_15(INPIN);
  TURN_OFF_PC_PUD_0_15(OUTPIN);
  SET_PC_INP_GPIO(INPIN);
  SET_PC_INP_GPIO(OUTPIN); /* must use SET_PC_INP_GPIO() before SET_PC_OUT_GPIO() */
  SET_PC_OUT_GPIO(OUTPIN);

  return(0);
}

void free_gpio(){
  if(map_base){
    iounmap((void*)map_base);
  }
  /*
  gpio_free(INPIN);
  gpio_free(OUTPIN);
  */
}

void raiseFlag(void){
  static size_t raiseCounter = 0;
  
  if(raiseCounter%2==0)
    map_base[PC_DATA_REG_RELATIVE_ADDR/4] |= (1<<OUTPIN); /* set output to High */
  else
    map_base[PC_DATA_REG_RELATIVE_ADDR/4] ^= (1<<OUTPIN); /* set output to Low */
  raiseCounter++;
}

bool gpioEventDetected(void){
  static size_t eventCounter = 0;
  if(((eventCounter%2==0) && READ_PC_PIN_VALUE(INPIN)) || ((eventCounter%2!=0) && (!READ_PC_PIN_VALUE(INPIN)))){
    eventCounter++;
    return(true);
  }
  return(false);
}

bool gpioEventDetected_v2(void){
  static unsigned long previous_state = 0;
  unsigned long cur_state;
  if((cur_state=READ_PC_PIN_VALUE(INPIN))!=previous_state){
    previous_state = cur_state;
    return(true);
  }
  return(false);
}

unsigned short searchValidMsgInBuffer(unsigned int buff){
  static bool foundPrefix = false;
  static short bitsAfterPrefix = 0;
  unsigned char buffTail = buff;
  buffTail<<=4; /* keep only last 4 bits - same size as prefix */
  buffTail>>=4; /* reset last 4 bits to their initial positions */

  if(PREFIX==buffTail){
    foundPrefix=true;
    bitsAfterPrefix=0;
    return(0);
  }
  else if(foundPrefix){
    bitsAfterPrefix++;
    if(bitsAfterPrefix==12){
      foundPrefix=false;
      return((unsigned short)buff);
    }
  }
  return(0);
}

bool msgToSentInBuffer(unsigned short msgToSent, unsigned int buff){
  unsigned short tmpSubBuffer = buff;
  if(msgToSent==tmpSubBuffer)
    return(true);
  return(false);
}

int alloc_line(char **lineptr, size_t *n){
  if (!lineptr || !n)
    return -1;
  if (!*lineptr) {
    *n = MIN_CHUNK;
    *lineptr = kmalloc(*n, GFP_KERNEL);
    if (!*lineptr)
      return -1;
  }
  memset(*lineptr, 0, *n);
  return 0;
}

int realloc_line(char **lineptr, size_t *n){
  void *tmp;

  if (*n > MIN_CHUNK)
    *n *= 2;
  else
    *n += MIN_CHUNK;
  tmp = krealloc(*lineptr, *n, GFP_KERNEL);
  if (tmp == NULL)
    return -1;
  *lineptr = tmp;
  return 0;
}

int concat_buffer(char **lineptr, size_t *n, const char *buf, char *save, char terminator){
  uint i;
  uint j;

  i = strlen(*lineptr);
  j = 0;
  while(buf[j]){
    if(i == *n)
      if(realloc_line(lineptr, n) == -1)
        return -1;
      if(buf[j] == terminator || i == *n){
        (*lineptr)[i] = '\0';
        if(buf[j] == terminator)
          ++j;
      save = strcpy(save, buf + j);
      return i;
      }
    (*lineptr)[i++] = buf[j++];
  }
  save[0] = '\0';
  (*lineptr)[i] = '\0';
  return -1;
}

ssize_t kgetstr(char **lineptr, size_t *n, struct file *f, char terminator)
{
  static char save[BUFFER_SIZE];
  char buf[BUFFER_SIZE];
  ssize_t linelen;
  ssize_t readlen;

  if (alloc_line(lineptr, n) == -1)
    return -1;
      /* Saved buffer */
  if ((linelen = concat_buffer(lineptr, n, save, save, terminator)) != -1)
    return linelen;
  while ((readlen = kread(f, buf, BUFFER_SIZE - 1)) > 0) {
    buf[readlen] = '\0';
    if ((linelen = concat_buffer(lineptr, n, buf, save, terminator)) != -1)
      return linelen;
  }
      /* Partial line */
  if (*lineptr && (*lineptr)[0] != '\0')
    return strlen(*lineptr);
  return -1;
}

ssize_t kgetline(char **lineptr, size_t *n, struct file *f)
{
  return kgetstr(lineptr, n, f, '\n');
}
