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

#include <linux/time.h> /* to keep track of elapsed time */

#define MAXRECVSTRING 1200 /* Longest string to receive */
#define SUCCESSES_TO_FINISH 100
#define SERVER_PORT 37020
#define READING_STEP 500 /* 1000, 500, 333, 250, 200, 166, 142, 125, 111, 100, 90, 83, 76, 71, 66, 62, 58, 55, 52, 50 */
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

long double Microsecs(void){
  static struct timeval _t;  
  static struct timezone tz;  
  gettimeofday(&_t, &tz);

  return((long double)_t.tv_sec*(1000*1000) + (long double)_t.tv_usec);
}

typedef struct meandata{
  size_t samplesNum;
  double mean;
}meanData;

void meanCalc(meanData* meanAndCount, double newSample){
  /* use this function to dynamically calculate the rolling average and total number of samples */
  static size_t samples = 0;
  static double mean;
  samples++;
  if(samples == 1){
    mean = newSample;
    meanAndCount->mean = mean;
    meanAndCount->samplesNum = samples;
  }
  mean = mean + (newSample - mean)/samples;
  meanAndCount->mean = mean;
  meanAndCount->samplesNum = samples;
}

String* inputFileToString(const char* finName){
  struct file *fin;
  String* inFileData = (String*)kmalloc(sizeof(String), GFP_KERNEL);
  unsigned char *fileAsString = NULL;
  
  /* calcuate file size in bytes */
  struct kstat ks;
  int error;
  mm_segment_t old_fs = get_fs();

  set_fs(get_ds());
  error = vfs_stat(finName, &ks);
  inFileData->length = ks.size;
  /* printk(KERN_INFO "file size: %lld Bytes.\n", ks.size); */
  set_fs(old_fs);
  /* --------------------------- */
  fileAsString = (unsigned char *)kmalloc((ks.size)*sizeof(unsigned char), GFP_KERNEL);
  fin = kopen(finName, O_RDONLY, 0);
  if(fin == NULL)
    return(NULL);
  if(0>kread(fin, fileAsString, ks.size))
    return(NULL);

  kclose(fin);
  inFileData->string = fileAsString;
  inFileData->string[ks.size]='\0';

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

HashArrayObj* createAndFillHashArray(node_t* cbPtr, node_t* wHPtr){
  /* initialize hashArray */
  HashArrayObj* HashArray = (HashArrayObj*)kmalloc(sizeof(HashArrayObj), GFP_KERNEL);
  HashArray->arrayLength = cbPtr->arrayLength;
  HashArray->dataPtr = (HashData**)kmalloc((HashArray->arrayLength)*sizeof(HashData*), GFP_KERNEL);
  if((HashArray==NULL) || (HashArray->arrayLength==0) || (HashArray->dataPtr==NULL))
    return(NULL);
  size_t dummy;
  for(dummy=0; dummy<HashArray->arrayLength; dummy++)
    HashArray->dataPtr[dummy]=NULL;
  /* -------------------- */

  /* fill hash array with key-data pairs */
  unsigned short i;
  for(i=0; i<256; i++)
    addItemToArray(i, cbPtr->arrayPtr[i], HashArray);
  for(i=256; i<HashArray->arrayLength-1; i++) /* use this -1 to leave a place in the table for a NULL item, in order to avoid infinite loops searching */
    addItemToArray(wHPtr->arrayPtr[i-256], cbPtr->arrayPtr[i], HashArray);
  /* ----------------------------------- */
  return(HashArray);
}

void displayhashes(HashArrayObj* hashMapArray){
  size_t i = 0;
  while(i<hashMapArray->arrayLength){
    if(hashMapArray->dataPtr[i]==NULL)
      printk(" (NULL, NULL)")
    else
      printk(KERN_INFO " (%hu,%hu)",hashMapArray->dataPtr[i]->key,hashMapArray->dataPtr[i]->data);
    i++;
  }
  printk("\n");
}

HashData *search(unsigned short key, HashArrayObj* hashArray){
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
    /* printk(KERN_INFO "\n\n(%hu, %hu)\n", word, item->data); */
    return(item->data); /* msgToSend = item->data; */
  }
  return(0);
}

unsigned short getNextMsgToSend(HashArrayObj* hMapArray, String* fAsString){
  static size_t current_pos = 0;
  if(current_pos>=fAsString->length)
    return(TERMINATION_MESSAGE); /* to indicate end of file exchange. */

  unsigned short current_buffer = (fAsString->string[current_pos] << 8) + fAsString->string[current_pos+1];
  unsigned short msgFound = 0;
  if((msgFound=msgInWord(current_buffer, hMapArray))!=0){
    current_pos+=2;
    return(msgFound);
  }
  else{ /* if msgFound==0 which means there is no match from hashCode for 2-byte msg */
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
  res =  kernel_recvmsg(sock, header, &vec, 1, size_buff, MSG_WAITALL);
  set_fs(oldmm);

  return(res);
}

static int __init bitSurfing_Receiver_init(void){
  long double moduleInTime = Microsecs();
  /* Setup Input and Output Pins */
  if(init_gpio() != 0)
    return(-EINVAL);
  /* --------------------------- */

  /* -------- Setup UDP Socket -------- */
  struct sockaddr_in broadcastAddr;
  struct socket *sock;
  struct msghdr hdr;

  if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock) < 0){
    printk( KERN_ERR "Error while creating socket.\n");
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

  /* Load CodeBook and HashWordBook */
  node_t *codebookPtr = NULL, *wordHashesPtr = NULL;
  if((((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt", ISCODEBOOK)) == NULL) || codebookPtr->arrayLength == 0) || 
    (((wordHashesPtr = OpenAndStoreCodebookToVector("codebook1.txt", ISHASHWORD)) == NULL) || wordHashesPtr->arrayLength == 0)){
    printk(KERN_ERR "Failed to store CodeBook or HashWords to Vector.\n");
    sock_release(sock);
    free_gpio();
    return(-EINVAL);
  }
  printk("codebook len :%zu", codebookPtr->arrayLength);
  printk("wordHashesPtr len : %zu", wordHashesPtr->arrayLength);
  /* ---------------------------------- */
  
  /* Create the HashMap */
  HashArrayObj* hashMapArray = createAndFillHashArray(codebookPtr);
  //displayhashes(hashMapArray);
  /*
  HashData * test = searchValue(58976, hashMapArray);
  printk(KERN_INFO "\n\n(%hu, %hu)\n", test->key, test->data);
  printk(KERN_INFO "\n\n%hu\n" , msgInWord(0b0000001111101110, hashMapArray));
  printk(KERN_INFO "%hu\n", 0b0000001111101110);
  */
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

  unsigned int buffer = 0; /* Node Buffer of int(=32bit) size */
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
  unsigned int foundMsgsCache = 0;
  unsigned short sentMsgs[SUCCESSES_TO_FINISH];
  int bits, byte, receivedBytesLen;
  long double foundMsgTime, prevFoundMsgTime;
  meanData *meanDataPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  if(meanDataPtr==NULL)
    printk(KERN_ERR "Failed to allocate memmory for meanDataPtr\n")
  meanDataPtr->samplesNum = 0;

  printk(KERN_INFO "Ready to receive...\n");
  while((successCounter<SUCCESSES_TO_FINISH) && (successReceivedCounter<SUCCESSES_TO_FINISH)){
    if((receivedBytesLen = udp_receive(sock, &hdr, receivedByte, sizeof(receivedByte))) < 0){
      printk(KERN_ERR "recvfrom() failed. Terminating...\n");
      break;
    }
    if(meanDataPtr->samplesNum == 0)
      foundMsgTime = Microsecs(); /* need this for the first time a Msg is found */
    if(terminationFlag==true)
      break;

    for(byte=PACKET_HEADERS_SIZE_BEGIN; byte<MAXRECVSTRING; byte+=READING_STEP){
      buffer = (buffer << 8) + receivedByte[byte];

      for (bits = 7; bits >= 0; bits--){
        x_bits_till_next_msg++;
        if((foundMsg = searchValidMsgInBuffer(buffer>>bits))!=0){
          /* timing related and valid words number tracking */
          prevFoundMsgTime = foundMsgTime;
          foundMsgTime = Microsecs();
          meanCalc(meanDataPtr, (foundMsgTime-prevFoundMsgTime));
          /* ---------------------------------------------- */
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
  printk(KERN_INFO "Total number of valid Messages count = %zu, providing a rolling average of %fuseconds for valid message arrival.\n",meanDataPtr->samplesNum, meanDataPtr->mean);
  /* Open both sending and receiving log files and write data. */
  struct file *sendingfd = kopen("sendBPi.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  struct file *receivingfd = kopen("recBPi.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);

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
  if(meanDataPtr){
    kfree(meanDataPtr);
  }
  
  free_gpio();
  
  long double moduleTotalTime = Microsecs() - moduleInTime;
  printk(KERN_INFO "Module took %lf seconds to finish.\n", moduleTotalTime/(1000*1000));
  return(0);
}

static void __exit bitSurfing_Receiver_exit(void){
  printk(KERN_INFO "Finished!\n");
}

module_init(bitSurfing_Receiver_init);
module_exit(bitSurfing_Receiver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dimitris");
MODULE_DESCRIPTION("BitSurfing BPi Receiving Node");
MODULE_VERSION("1.0");
