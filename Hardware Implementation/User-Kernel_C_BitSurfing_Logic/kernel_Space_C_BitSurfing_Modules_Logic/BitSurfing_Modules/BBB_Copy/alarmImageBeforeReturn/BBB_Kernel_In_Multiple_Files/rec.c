#include <linux/init.h> /* for module_init() and module_exit() */
#include <linux/module.h> /* for MODULE_<any> */
#include <linux/kernel.h> /* for KERN_INFO */
#include <linux/slab.h> /* for kmalloc() */
/* from git repo, for kgetline() and other I/O */
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include "kfile.h"
#include "gpioStuff.h"
/* ------------- */
#include<linux/string.h>  /* for memset() */
#include <net/sock.h> /* UDP socket */
#include <linux/types.h> /* for bool and other types */

#define MAXRECVSTRING 1200 /* Longest string to receive */
#define SUCCESSES_TO_FINISH 10
#define SERVER_PORT 37020
#define READING_STEP 166
#define PACKET_HEADERS_SIZE_BEGIN 200

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

static int __init bitSurfing_Receiver_init(void){

  /* Setup Input and Output Pins */
  if(init_gpio() != 0){
    printk( KERN_ERR "ioremap failed.\n Are you root?\n");
    return(-EINVAL);
  }
  /* --------------------------- */

  struct sockaddr_in broadcastAddr;
  struct socket *sock;
  struct msghdr hdr;

  /* -------- Setup UDP Socket -------- */
  if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
    printk( KERN_ERR "server: Error creating recvsocket\n");
    free_gpio();
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
    free_gpio();
    return(-EINVAL);
  }
  /* ---------------------------------- */

  /* Load CodeBook and randomly choose a message of interest */
  node_t *codebookPtr = NULL;
  if(((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL) || codebookPtr->arrayLength == 0){
    printk(KERN_ERR "Failed to store CodeBook to Vector.\n");
    sock_release(sock);
    free_gpio();
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
    
    for(byte=PACKET_HEADERS_SIZE_BEGIN; byte<MAXRECVSTRING; byte+=READING_STEP){
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
        if(gpioEventDetected()){
          receivedMsgs[successReceivedCounter] = foundMsgsCache;
          oldReceivedMsgs[successReceivedCounter] = (foundMsgsCache >> 16);
          successReceivedCounter++;
          cacheNextMsg=true;
          x_bits_till_next_msg = 0;
        }
        else{
          if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
            raiseFlag();
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
    free_gpio();
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

  if(sock){
    sock_release(sock);
  }
  if(codebookPtr->arrayPtr){
    kfree(codebookPtr->arrayPtr);
  }
  if(codebookPtr){
    kfree(codebookPtr);
  }
  
  free_gpio();
  
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
