#include <linux/init.h> /* for module_init() and module_exit() */
#include <linux/module.h> /* for MODULE_<any> */
#include <linux/kernel.h> /* for KERN_INFO */
#include <linux/slab.h> /* for kmalloc() */
/* from git repo, for kgetline() and other I/O */
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include "kfile.h"
#include "kgetline.h"
/* ------------- */
#include<linux/string.h>  /* for memset() */
#include <linux/random.h> /* for get_random_bytes() */
#include <net/sock.h> /* UDP socket */
#include <linux/types.h> /* for bool and other types */
#include <asm/io.h> /* for direct register access : ioremap() and iounmap() */
#include <linux/gpio.h> /* for gpio_is_valid(), gpio_request() and gpio_free() */

#define MAXRECVSTRING 1200 /* Longest string to receive */
#define SUCCESSES_TO_FINISH 20
#define SERVER_PORT 37020
#define PREFIX 0b1110
#define READING_STEP 166
#define PACKET_HEADERS_SIZE 200

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

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

void construct_header(struct msghdr * msg, struct sockaddr_in * address){
  msg->msg_name    = address;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;
  msg->msg_flags = 0;
}

int udp_receive(struct socket *sock, struct msghdr * header, void * buff, size_t size_buff){
  struct kvec vec;
  mm_segment_t oldmm;
  int res;

  vec.iov_len = size_buff;
  vec.iov_base = buff;


  oldmm = get_fs(); set_fs(KERNEL_DS);
  res =  kernel_recvmsg(sock, header, &vec, 1, size_buff, MSG_WAITALL); //MSG_DONTWAIT
  set_fs(oldmm);

  return(res);
}

unsigned short binary2short(char* s){
  char *p = s;
  unsigned short r = 0;
  unsigned char c;

  while(p && *p){
    c = *p++;
    if(c == '0') r = (r<<1);
    else if(c == '1') r = (r<<1)+1;
    else break;
  }
  return(r);
}

node_t * OpenAndStoreCodebookToVector(char *filename){
  size_t fileLines = 0;
  size_t dummy = 0;
  unsigned short *cbPtr = NULL;
  node_t* codeBPtr = (node_t*)kmalloc(sizeof(node_t), GFP_KERNEL);

  struct file *fin;
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;
  fin = kopen(filename, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
    
  while((read = kgetline(&line, &len, fin)) != -1)
    fileLines++;
  kclose(fin);

  cbPtr = (unsigned short *)kmalloc(fileLines*sizeof(unsigned short), GFP_KERNEL);
  if(cbPtr==NULL){
    printk("Couldn't allocate memmory\n");
    return(NULL);
  }
  fin = kopen(filename, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
  while(((read = kgetline(&line, &len, fin)) != -1) && (dummy < fileLines))
    cbPtr[dummy++] = binary2short(line);

  if(line != NULL)
    kfree(line);
  kclose(fin);
  codeBPtr->arrayPtr = cbPtr;
  codeBPtr->arrayLength = fileLines;

  return(codeBPtr);
}

unsigned short pickRandomMsgToSend(node_t *cBPtr){
  unsigned int randInt;
  get_random_bytes(&randInt, sizeof(randInt));
  size_t randomIndex = (randInt % (cBPtr->arrayLength)) + 0;
  return(cBPtr->arrayPtr[randomIndex]);
}

/*ssize_t kwrite(struct file *file, char __user *buf, size_t count){
  mm_segment_t oldfs;
  ssize_t ret;

  oldfs = get_fs();
  set_fs(get_ds());
  ret = vfs_write(file, buf, count, &file->f_pos);
  set_fs(oldfs);
  return ret;
}*/

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

bool gpioEventDetected(volatile unsigned long *map_base){
  static size_t eventCounter = 0;
  if(((eventCounter%2==0) && GET_GPIO(INPIN)) || ((eventCounter%2!=0) && (!GET_GPIO(INPIN)))){
    eventCounter++;
    return(true);
  }
  return(false);
}

void raiseFlag(volatile unsigned long *map_base){
    static size_t raiseCounter = 0;
    if(raiseCounter%2==0)
        map_base[GPIO_DATAOUT/4]  |= (1 << OUTPIN);  // set output to HIGH
    else
        map_base[GPIO_DATAOUT/4]  ^= (1 << OUTPIN);  // set output to LOW
    raiseCounter++;
}

static int __init bitSurfing_Receiver_init(void){

  /* Setup Input and Output Pins */
  volatile unsigned long *map_base = (volatile unsigned long*)ioremap(GPIO2_BASE_ADDR, GPIO2BANK_SIZEINBYTES);
  
  if(((void*)map_base) == NULL){
    printk( KERN_ERR "ioremap failed.\n Are you root?\n");
    return(-EINVAL);
  }
  if(!gpio_is_valid(INPINSYSFS) || !gpio_is_valid(OUTPINSYSFS) || gpio_request(INPINSYSFS, "INPIN") || gpio_request(OUTPINSYSFS, "OUTPIN")){
    printk(KERN_CRIT "GPIO initialization failed.\n"); // /sys/class/gpio/export
    return(-EINVAL);
  }

  map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << OUTPIN)); // setup output pin
  map_base[OE_ADDR/4] |= (1 << INPIN);    // setup input pin
  /* --------------------------- */

  struct sockaddr_in broadcastAddr;
  struct socket *sock;
  struct msghdr hdr;

  /* -------- Setup UDP Socket -------- */
  if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
    printk( KERN_ERR "server: Error creating recvsocket\n");
    gpio_free(INPIN);
    gpio_free(OUTPIN);
    return(-EINVAL);
  }

  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_port = htons((unsigned short)SERVER_PORT);
  construct_header(&hdr, &broadcastAddr);

  if(sock->ops->bind(sock, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0){
    printk(KERN_ERR "Error while binding socket to %pI4\n", &broadcastAddr.sin_addr);
    sock_release(sock);
    gpio_free(INPIN);
    gpio_free(OUTPIN);
    return(-EINVAL);
  }
  /* ---------------------------------- */

  /* Load CodeBook and randomly choose a message of interest */
  node_t *codebookPtr = NULL;
  if(((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL) || codebookPtr->arrayLength == 0){
    printk(KERN_ERR "Failed to store CodeBook to Vector.\n");
    sock_release(sock);
    gpio_free(INPIN);
    gpio_free(OUTPIN);
    return(-EINVAL);
  }
  printk("%zu", codebookPtr->arrayLength);
  unsigned short msgToSent = pickRandomMsgToSend(codebookPtr);
  printk(KERN_INFO "Selected Random Message : %hu\n",msgToSent);
  /* ------------------------------------------------------- */

  unsigned int buffer = 0;    /* Node Buffer of int(=32bit) size */
  unsigned char receivedByte[MAXRECVSTRING];
  unsigned short foundMsg = 0;
  unsigned short successCounter = 0;
  unsigned short successReceivedCounter = 0;
  unsigned short receivedMsgs[SUCCESSES_TO_FINISH];
  unsigned short oldReceivedMsgs[SUCCESSES_TO_FINISH];
  unsigned short afterReceivedMsgs[SUCCESSES_TO_FINISH];
  bool cacheNextMsg = false;
  size_t x_bits_till_next_msg = 0;
  size_t x_bits_til_next_valid_msg[SUCCESSES_TO_FINISH];
  unsigned int   foundMsgsCache = 0;
  unsigned short sentMsgs[SUCCESSES_TO_FINISH];
  int bits, byte, receivedBytesLen;

  printk(KERN_INFO "Ready to receive...\n");

  while((successCounter<SUCCESSES_TO_FINISH) && (successReceivedCounter<SUCCESSES_TO_FINISH)){
    if((receivedBytesLen = udp_receive(sock, &hdr, receivedByte, sizeof(receivedByte))) < 0){
      printk(KERN_ERR "recvfrom() failed. Terminating...\n");
      break;
    }
    
    for(byte=PACKET_HEADERS_SIZE; byte<MAXRECVSTRING; byte+=READING_STEP){
      buffer = (buffer << 8) + receivedByte[byte];

    for (bits = 7; bits >= 0; bits--){
      x_bits_till_next_msg++;
      if((foundMsg = searchValidMsgInBuffer(buffer>>bits))!=0){
        foundMsgsCache = (foundMsgsCache << 16) + foundMsg;
        if(cacheNextMsg){
          afterReceivedMsgs[successReceivedCounter-1]=foundMsg;
          cacheNextMsg=false;
          x_bits_til_next_valid_msg[successReceivedCounter-1]=x_bits_till_next_msg;
        }
      }
      if(gpioEventDetected(map_base)){
        receivedMsgs[successReceivedCounter] = foundMsgsCache;
        oldReceivedMsgs[successReceivedCounter] = (foundMsgsCache >> 16);
        successReceivedCounter++;
        cacheNextMsg=true;
        x_bits_till_next_msg = 0;
      }
      else{
        if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
          raiseFlag(map_base);
          sentMsgs[successCounter] = msgToSent;
          msgToSent = pickRandomMsgToSend(codebookPtr);
          successCounter++;
          printk("Success!\n");
        }
      }
    }
    }
  }
  /* Open both sending and receiving log files and write data. */
  struct file *sendingfd = kopen("sendBBB.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  struct file *receivingfd = kopen("recBBB.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);

  if((sendingfd==NULL) || (receivingfd)==NULL){
    printk("Couldn't open log Files to write data.\n");
    sock_release(sock);
    gpio_free(INPIN);
    gpio_free(OUTPIN);
    return(-EINVAL);
  }

  int i=0;
  char logBuffer[100];

  for(i=0;i<successReceivedCounter;i++){
    snprintf(logBuffer, 100, "%hu | %hu | %hu | bitsPassedTilMsg: %zu\n", receivedMsgs[i], oldReceivedMsgs[i], afterReceivedMsgs[i], x_bits_til_next_valid_msg[i]);
    kwrite(receivingfd, logBuffer, strlen(logBuffer));
  }
  for(i=0;i<successCounter;i++){
    snprintf(logBuffer, 100, "%hu\n", sentMsgs[i]);
    kwrite(sendingfd, logBuffer, strlen(logBuffer));
  }
  kclose(sendingfd);
  kclose(receivingfd);
  /* --------------------------------------------------------- */

  if(map_base){
    iounmap((void*)map_base);
  }
  if(sock){
    sock_release(sock);
  }
  if(codebookPtr){
    kfree(codebookPtr);
  }
  
  gpio_free(INPIN);
  gpio_free(OUTPIN);
  
  return(0);
}

static void __exit bitSurfing_Receiver_exit(void){
  printk(KERN_INFO "Finished!\n");
}

module_init(bitSurfing_Receiver_init);
module_exit(bitSurfing_Receiver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dimitris");
MODULE_DESCRIPTION("BitSurfing BBB Receiving Node");
MODULE_VERSION("1.0");

