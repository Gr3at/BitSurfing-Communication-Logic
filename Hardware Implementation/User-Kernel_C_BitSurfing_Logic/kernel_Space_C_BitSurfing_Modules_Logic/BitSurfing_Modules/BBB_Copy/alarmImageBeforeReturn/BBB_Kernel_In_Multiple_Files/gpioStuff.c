/*
** gpioStuff.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri Mar 14 21:29:24 2014 xsyann
** Last update Fri Apr 18 22:22:10 2014 xsyann
*/

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
/* Valid only for BeagleBoneBlack with AM335x ARM Cortex-A8 Microprocessor */
#define GPIO2_BASE_ADDR 0x481AC000
#define GPIO2BANK_SIZEINBYTES 4096
#define OE_ADDR 0x134
#define GPIO_DATAOUT 0x13C
#define GPIO_DATAIN 0x138
#define OUTPIN 2 // board pin P8.7  66
#define OUTPINSYSFS 66 // board pin P8.7  66
#define INPIN 5 // board pin P8.9   69
#define INPINSYSFS 69 // board pin P8.9   69
/* ground is board pin P8.1 */
#define GET_GPIO(g) (map_base[GPIO_DATAIN/4]&(1<<g)) // 0 if LOW, (1<<g) if HIGH
/* ----------------------------------------------------------------------- */

volatile unsigned long *map_base = NULL;

int init_gpio(void){
  map_base = (volatile unsigned long*)ioremap(GPIO2_BASE_ADDR, GPIO2BANK_SIZEINBYTES);
  
  if(((void*)map_base) == NULL){
    printk( KERN_ERR "ioremap failed.\n Are you root?\n");
    return(-1);
  }

  if(!gpio_is_valid(INPINSYSFS) || !gpio_is_valid(OUTPINSYSFS) || gpio_request(INPINSYSFS, "INPIN") || gpio_request(OUTPINSYSFS, "OUTPIN")){
    printk(KERN_CRIT "GPIO initialization failed.\n"); // /sys/class/gpio/export
    return(-1);
  }

  map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << OUTPIN)); // setup output pin
  map_base[OE_ADDR/4] |= (1 << INPIN);    // setup input pin
  return(0);
}

void free_gpio(){
  if(map_base){
    iounmap((void*)map_base);
  }
  gpio_free(INPIN);
  gpio_free(OUTPIN);
}

void raiseFlag(void){
  static size_t raiseCounter = 0;
  
  if(raiseCounter%2==0)
    map_base[GPIO_DATAOUT/4]  |= (1 << OUTPIN);  // set output to HIGH
  else
    map_base[GPIO_DATAOUT/4]  ^= (1 << OUTPIN);  // set output to LOW
  raiseCounter++;
}

bool gpioEventDetected(void){
  static size_t eventCounter = 0;
  if(((eventCounter%2==0) && GET_GPIO(INPIN)) || ((eventCounter%2!=0) && (!GET_GPIO(INPIN)))){
    eventCounter++;
    return(true);
  }
  return(false);
}

unsigned short searchValidMsgInBuffer(unsigned int buff){
  static bool foundPrefix = false;
  static short bitsAfterPrefix = 0;
  unsigned char buffTail = buff;
  buffTail<<=4; // keep only last 4 bits
  buffTail>>=4; // reset last 4 bits to their initial positions

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
      if((buf[j] == terminator) || (i == *n)){
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
