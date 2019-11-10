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

#include <linux/uaccess.h>  /* for user access rights */
#include <linux/fs.h> /* for filesystem access */

#define MAXRECVSTRING 1200 /* Longest string to receive */
#define SUCCESSES_TO_FINISH 100
#define SERVER_PORT 37020
#define READING_STEP 500 // or 166 or 125 ...
#define PACKET_HEADERS_SIZE_BEGIN 200

#define TERMINATION_MESSAGE 61439 // which is the last valid message of codebook

typedef struct Hashdata{
  unsigned short data;   
  unsigned short key;
}HashData;

typedef struct Hasharray{
  size_t arrayLength;
  HashData **dataPtr;
}HashArrayObj;

typedef struct stringData{
  unsigned char* string;
  size_t length;
}String;

String* inputFileToString(const char* finName){
  struct file *fin;
  String* inFileData = (String*)kmalloc(sizeof(String), GFP_KERNEL);
  unsigned char *fileAsString = NULL;
  
  /* calcuate file size in bytes */
  struct kstat ks;
  int error;
  mm_segment_t old_fs = get_fs();

  set_fs(get_ds()); //  set_fs(get_ds()) == set_fs(KERNEL_DS); both are exactly the same
  error = vfs_stat(finName, &ks);
  inFileData->length = ks.size;
  //printk(KERN_INFO "size: %lld\n", ks.size);
  set_fs(old_fs);

  /* --------------------------------- */
  fileAsString = (unsigned char *)kmalloc((ks.size)*sizeof(unsigned char), GFP_KERNEL);
  fin = kopen(finName, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
  if(0>kread(fin, fileAsString, ks.size))
    return(NULL);

  kclose(fin);
  inFileData->string = fileAsString;
  inFileData->string[ks.size]='\0';
  //printk(KERN_INFO "%s\n", inFileData->string);

  return(inFileData);
}

unsigned short hashFunction(unsigned short key, size_t SIZE){
  return(key % SIZE);
}

void addItemToArray(unsigned short key, unsigned short data, HashArrayObj* hArray){
  unsigned short hindex = hashFunction(key, hArray->arrayLength);
  HashData* newItem = (HashData*)kmalloc(sizeof(HashData), GFP_KERNEL);
  while(hArray->dataPtr[hindex]!=NULL){
    hindex++;
    hindex %= hArray->arrayLength;
  }
  newItem->key = key;
  newItem->data = data;
  hArray->dataPtr[hindex] = newItem;
}

HashArrayObj* createAndFillHashArray(node_t* cbPtr){
  /* initialize hashArray */
  HashArrayObj* HashArray = (HashArrayObj*)kmalloc(sizeof(HashArrayObj), GFP_KERNEL);
  HashArray->arrayLength = cbPtr->arrayLength;
  HashArray->dataPtr = (HashData**)kmalloc((cbPtr->arrayLength)*sizeof(HashData*), GFP_KERNEL);
  if((HashArray==NULL) || (HashArray->arrayLength==0) || (HashArray->dataPtr==NULL))
    return(NULL);
  size_t dummy;
  for(dummy=0; dummy<cbPtr->arrayLength; dummy++)
    HashArray->dataPtr[dummy]=NULL;
  /* -------------------- */

  /* fill hash array with key-data pairs */
  unsigned short i;
  for(i=0; i<HashArray->arrayLength; i++)
    addItemToArray(i, cbPtr->arrayPtr[i], HashArray);
  /* ----------------------------------- */
  return(HashArray);
}

void displayhashes(HashArrayObj* hashMapArray){
  size_t i = 0;
  while(i<hashMapArray->arrayLength){
    printk(KERN_INFO " (%hu,%hu)",hashMapArray->dataPtr[i]->key,hashMapArray->dataPtr[i]->data);
    i++;
  }
  printk("\n");
}

HashData *search(unsigned short key, HashArrayObj* hashArray){
  if(key>=hashArray->arrayLength)
    return(NULL);
  unsigned short hIndex = hashFunction(key, hashArray->arrayLength);
  while(hashArray->dataPtr[hIndex] != NULL){
    if(hashArray->dataPtr[hIndex]->key == key)
       return(hashArray->dataPtr[hIndex]); 
    hIndex++;
    hIndex %= hashArray->arrayLength;
  }
  return(NULL);
}

HashData *searchValue(unsigned short value, HashArrayObj* hashArray){
  int i;
  for(i = 0; i<hashArray->arrayLength; i++){
    if((hashArray->dataPtr[i] != NULL) && (hashArray->dataPtr[i]->data==value))
       return(hashArray->dataPtr[i]);
  }
  return(NULL);
}

unsigned short msgInWord(unsigned short word, HashArrayObj* hashArray){
  HashData* item = search(word, hashArray);
  if((item != NULL) && (item->key == word)){
    //printk(KERN_INFO "\n\n(%hu, %hu)\n", word, item->data);
    return(item->data);  //msgToSend = item->data;
  }
  return(0);
}

unsigned short getNextMsgToSend(HashArrayObj* hMapArray, String* fAsString){
  static size_t current_pos = 0;
  if(current_pos>=fAsString->length)
    return(TERMINATION_MESSAGE); // to indicate end of file exchange procedure

  unsigned short current_buffer = (fAsString->string[current_pos] << 8) + fAsString->string[current_pos+1];
  unsigned short msgFound = 0;
  if((msgFound=msgInWord(current_buffer, hMapArray))!=0){
    current_pos+=2;
    return(msgFound);
  }
  else{ // if msgFound==0 which means there is no match from hashCode for 2-byte msg
    current_buffer = (0 << 8) + fAsString->string[current_pos];
    if((msgFound=msgInWord(current_buffer, hMapArray))==0){
      printk(KERN_ERR "No message Found to Send!! Check getNextMsgToSend() function. \n");
      return(0);
    }
    current_pos++;
    return(msgFound);
  }
}

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

  /* -------- Setup UDP Socket -------- */
  struct sockaddr_in broadcastAddr;
  struct socket *sock;
  struct msghdr hdr;

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
  /* ------------------------------------------------------- */
  
  /* Create the HashMap */
  HashArrayObj* hashMapArray = createAndFillHashArray(codebookPtr);
  //displayhashes(hashMapArray);
  HashData * test = searchValue(58976, hashMapArray);
  printk(KERN_INFO "\n\n(%hu, %hu)\n", test->key, test->data);
  printk(KERN_INFO "\n\n%hu\n" , msgInWord(0b0000001111101110, hashMapArray));
  printk(KERN_INFO "%hu\n", 0b0000001111101110);
  /* ------------------ */

  /* Store input file to string */
  String* fileAsString = inputFileToString("testInput.txt");
  if(fileAsString==NULL){
    printk(KERN_ERR "Failed to store input file to String.\n");
    sock_release(sock);
    free_gpio();
    kfree(codebookPtr->arrayPtr);
    kfree(codebookPtr);
    return(-EINVAL);
  }

  unsigned short msgToSent = getNextMsgToSend(hashMapArray, fileAsString);
  printk(KERN_INFO "1st message to send : %hu\n", msgToSent);
  /* -------------------------- */

  unsigned int buffer = 0;    /* Node Buffer of int(=32bit) size */
  unsigned char receivedByte[MAXRECVSTRING];
  unsigned short foundMsg = 0;
  unsigned short successCounter = 0;
  unsigned short successReceivedCounter = 0;
  unsigned short receivedMsgs[SUCCESSES_TO_FINISH];
  unsigned short oldReceivedMsgs[SUCCESSES_TO_FINISH];
  unsigned short afterReceivedMsgs[SUCCESSES_TO_FINISH];
  bool cacheNextMsg = false, terminationFlag=false;
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
    if(terminationFlag==true)
      break;

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
            if(foundMsg==TERMINATION_MESSAGE){
              terminationFlag=true;
              break;
            }
          }
        }
        if(gpioEventDetected()){
          receivedMsgs[successReceivedCounter] = foundMsgsCache;
          oldReceivedMsgs[successReceivedCounter] = (foundMsgsCache >> 16);
          successReceivedCounter++;
          cacheNextMsg=true;
          x_bits_till_next_msg = 0;
          if(foundMsgsCache==TERMINATION_MESSAGE){
            terminationFlag=true;
            break;
          }
        }
        else{
          if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
            raiseFlag();
            sentMsgs[successCounter] = msgToSent;
            if(msgToSent==TERMINATION_MESSAGE){
              successCounter++;
              printk(KERN_INFO "Sent last Message: %hu\n", msgToSent);
              terminationFlag=true;
              break;
            }
            msgToSent = getNextMsgToSend(hashMapArray, fileAsString);
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
  if(fileAsString->string){
    kfree(fileAsString->string);
  }
  if(fileAsString){
    kfree(fileAsString);
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
