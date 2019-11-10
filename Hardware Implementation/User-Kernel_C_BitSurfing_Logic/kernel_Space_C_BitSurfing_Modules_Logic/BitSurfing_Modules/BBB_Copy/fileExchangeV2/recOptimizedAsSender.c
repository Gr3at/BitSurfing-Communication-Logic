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

#include <linux/ktime.h> /* to keep track of elapsed time */

#define MAXRECVSTRING 1200 /* Longest string to receive */
#define SUCCESSES_TO_FINISH 150 /* not really needed if there is a file to exchange. */
#define SERVER_PORT 37020
#define READING_STEP 1 /* 1000, 500, 333, 250, 200, 166, 142, 125, 111, 100, 90, 83, 76, 71, 66, 62, 58, 55, 52, 50 */
#define PACKET_HEADERS_SIZE_BEGIN 200

#define WAIT_X_BITS_FOR_MSG 20
#define TERMINATION_MESSAGE 61439 /* or codebookPtr->arrayPtr[codebookPtr->arrayLength] - which is the last valid message of codebook */
#define SECONDSTOWAITBEFOREBREAK 600 /* aka 10mins */
#define DATA2COLLECT 10000
#define CPUFREQUENCY 1200 /* in MHz */
#define REFERENCECPUFREQ 1000 /* in MHz */
#define TRAINPERIOD 100 /* packets to receive during training period */

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

typedef struct meandata{
  size_t samplesNum;
  long long int mean;
}meanData;

long long int mydiv(long long int dividend, long long int divisor){
  long long int counter = 0;

  if(divisor==0)
    return(LLONG_MAX); /* LLONG_MAX defined in linux kernel.h */
  if(dividend<0)
    return(-mydiv(-dividend, divisor));
  if(divisor<0)
    return(-mydiv(dividend, -divisor));
  
  while(dividend>=divisor){
    dividend = dividend - divisor;
    counter++;
  }
  return(counter);
}

void meanCalc(meanData* meanAndCount, long long int newSample){
  /* use this function to dynamically calculate the rolling average and total number of samples */
  size_t samples = meanAndCount->samplesNum;
  long long int mean;
  samples++;
  if(samples == 1){
    mean = newSample;
    meanAndCount->mean = mean;
    meanAndCount->samplesNum = samples;
  }
  else{
    mean = meanAndCount->mean;
    mean = mean + mydiv((newSample - mean), samples);
    meanAndCount->mean = mean;
    meanAndCount->samplesNum = samples;
  }
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
  /* not really needed as a function but it's better to exist for potential future changes */
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
  for(i=0; i<256; i++) /* 0-255 contains all possible values for char */
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
      printk(" (NULL, NULL)");
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
  unsigned int i;
  for(i = 0; i<hashArray->arrayLength; i++){
    if((hashArray->dataPtr[i] != NULL) && (hashArray->dataPtr[i]->data==value))
       return(hashArray->dataPtr[i]);
  }
  return(NULL);
}

unsigned short msgInWord(unsigned short word, HashArrayObj* hashArray){
  HashData* item = search(word, hashArray);
  if((item != NULL) && (item->key == word))
    return(item->data); /* msgToSend = item->data; */
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
  ktime_t moduleInTime = ktime_get_real();
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
    (((wordHashesPtr = OpenAndStoreCodebookToVector("freqs.txt", ISHASHWORD)) == NULL) || wordHashesPtr->arrayLength == 0)){
    printk(KERN_ERR "Failed to store CodeBook or HashWords to Vector.\n");
    sock_release(sock);
    free_gpio();
    return(-EINVAL);
  }
  printk("codebook len :%zu", codebookPtr->arrayLength);
  printk("wordHashesPtr len : %zu", wordHashesPtr->arrayLength);
  /* ---------------------------------- */
  
  /* Create the HashMap */
  HashArrayObj* hashMapArray = createAndFillHashArray(codebookPtr, wordHashesPtr);
  //displayhashes(hashMapArray);
  /*
  HashData * test = searchValue(58976, hashMapArray);
  printk(KERN_INFO "\n\n(%hu, %hu)\n", test->key, test->data);
  printk(KERN_INFO "\n\n%hu\n" , msgInWord(0b0000001111101110, hashMapArray));
  printk(KERN_INFO "%hu\n", 0b0000001111101110);
  */
  /* ------------------ */

  /* Store input file to string */
  String* fileAsString = inputFileToString("file2exchange.txt");
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
  unsigned short foundMsg = 0, successCounter = 0, successReceivedCounter = 0;
  unsigned short receivedMsgs[SUCCESSES_TO_FINISH], oldReceivedMsgs[SUCCESSES_TO_FINISH], afterReceivedMsgs[SUCCESSES_TO_FINISH];
  bool myByteFlag = false, cacheNextMsg = false, terminationFlag=false;
  size_t trainCounter = 0, myByteFlagCounter = 0, x_bits_till_next_msg = 0, packetCounter=0, forLoopCounter=0, byteCounter, numOfBytes;
  size_t x_bits_til_next_valid_msg[SUCCESSES_TO_FINISH];
  unsigned int foundMsgsCache = 0;
  unsigned short sentMsgs[SUCCESSES_TO_FINISH];
  int bits, byte, receivedBytesLen;
  long long int delayToIntroduce, BitForLoopProcessingTime = 0, cpuDivisionDiff, availableTimeSlot, timePerByte;
  ktime_t foundMsgTime=ktime_get_real(), prevFoundMsgTime, NoMsgBreak=ktime_get_real(), sendMsgTime=ktime_get_real(), prevSendMsgTime;
  ktime_t prePacketTime, afterPacketTime, proccessingTime, afterProccessingTime, startForLoopTimer, afterForLoopTimer;
  long long int * PacketInterrarivalTime= (long long int *)kmalloc(DATA2COLLECT*sizeof(long long int), GFP_KERNEL);
  long long int * ProccessingInterrarivalTime= (long long int *)kmalloc(DATA2COLLECT*sizeof(long long int), GFP_KERNEL);
  long long int * ProccessingForLoopInterrarivalTime= (long long int *)kmalloc(DATA2COLLECT*sizeof(long long int), GFP_KERNEL);
  if(PacketInterrarivalTime==NULL || ProccessingInterrarivalTime==NULL || ProccessingForLoopInterrarivalTime==NULL)
    return(-1);

  unsigned short *byteOfMsg = (unsigned short *)kmalloc(70*sizeof(unsigned short), GFP_KERNEL);
  long long int * myByteFlagTimes = (long long int *)kmalloc(70*sizeof(long long int), GFP_KERNEL);
  
  meanData *meanDataPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  meanData *meanSendMsgDataPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  meanData *meanPacketPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  meanData *meanProccessingPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  meanData *meanProccessingForLoopPtr = (meanData *)kmalloc(sizeof(meanData), GFP_KERNEL);
  if((meanDataPtr==NULL) || (meanSendMsgDataPtr==NULL) || (meanPacketPtr==NULL) || (meanProccessingPtr==NULL) || (meanProccessingForLoopPtr==NULL))
    printk(KERN_ERR "Failed to allocate memmory for meanDataPtr or other pointers.\n");
  meanDataPtr->samplesNum = 0;
  meanSendMsgDataPtr->samplesNum = 0;
  meanPacketPtr->samplesNum = 0;
  meanProccessingPtr->samplesNum = 0;
  meanProccessingForLoopPtr->samplesNum = 0;

  /* Training Phase */
  while(trainCounter < TRAINPERIOD){
    /* Packet Reception */
    prePacketTime = ktime_get_real();
    if((receivedBytesLen = udp_receive(sock, &hdr, receivedByte, sizeof(receivedByte))) < 0){
      printk(KERN_ERR "recvfrom() failed. Terminating...\n");
      break;
    }
    afterPacketTime = ktime_get_real();
    if(trainCounter /*!=0*/)
      meanCalc(meanPacketPtr, ktime_us_delta(afterPacketTime,prePacketTime));
    trainCounter++;
    /* ---------------- */
  }
  numOfBytes = mydiv(MAXRECVSTRING-PACKET_HEADERS_SIZE_BEGIN, READING_STEP);
  availableTimeSlot = (meanPacketPtr->mean) - mydiv(10*(meanPacketPtr->mean),100); /* DT - 10%DT */
  timePerByte = mydiv(availableTimeSlot, numOfBytes); /* in usec */
  /* -------------- */
  printk(KERN_INFO "Finished Training...\nStarting actual communication\n");
  
  while((successCounter<SUCCESSES_TO_FINISH) && (successReceivedCounter<SUCCESSES_TO_FINISH)){
    /* Packet Reception */
    prePacketTime = ktime_get_real();
    if((receivedBytesLen = udp_receive(sock, &hdr, receivedByte, sizeof(receivedByte))) < 0){
      printk(KERN_ERR "recvfrom() failed. Terminating...\n");
      break;
    }
    afterPacketTime = ktime_get_real();
    if(packetCounter<DATA2COLLECT)
      PacketInterrarivalTime[packetCounter] = ktime_us_delta(afterPacketTime,prePacketTime);
    meanCalc(meanPacketPtr, ktime_us_delta(afterPacketTime,prePacketTime));
    /* ---------------- */
    if((terminationFlag==true)) // || (ktime_after(foundMsgTime, NoMsgBreak) /* if foundMsgTime > NoMsgBreak - ensure positive sub */ && ktime_ms_delta(foundMsgTime, NoMsgBreak)>((long long int ) (SECONDSTOWAITBEFOREBREAK*1000))))
      break;

    proccessingTime = ktime_get_real();
    byteCounter = 0; /* keep track of the #bytes processed */
    for(byte=PACKET_HEADERS_SIZE_BEGIN; byte<MAXRECVSTRING; byte+=READING_STEP){
      buffer = (buffer << 8) + receivedByte[byte];
      startForLoopTimer = ktime_get_real();

      for (bits = 7; bits >= 0; bits--){
        x_bits_till_next_msg++;
        if((foundMsg = searchValidMsgInBuffer(buffer>>bits))!=0){
          /* timing related and valid words number tracking */
          prevFoundMsgTime = foundMsgTime;
          foundMsgTime = ktime_get_real();
          meanCalc(meanDataPtr, ktime_us_delta(foundMsgTime,prevFoundMsgTime));
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
        /*if(gpioEventDetected()){
          NoMsgBreak = ktime_get_real();
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
        else{*/
          if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
            raiseFlag();
            myByteFlag = true;
            /* timing related and send messages number tracking */
            prevSendMsgTime = sendMsgTime;
            sendMsgTime = ktime_get_real();
            meanCalc(meanSendMsgDataPtr, ktime_ms_delta(sendMsgTime,prevSendMsgTime));
            byteOfMsg[successCounter] = byteCounter;
            /* ---------------------------------------------- */
            sentMsgs[successCounter] = msgToSent;
            if(msgToSent==TERMINATION_MESSAGE){
              successCounter++;
              printk(KERN_INFO "Sent last Message: %hu\n", msgToSent);
              terminationFlag=true;
              break;
            }
            msgToSent = getNextMsgToSend(hashMapArray, fileAsString);
            successCounter++;
            if(successCounter%20==0) /* just to ensure minimum delay because of printk() call... if i remember right printk() introduces order of several usec */
              printk("successCounter %hu!\n", successCounter);
          }
        //}
      }
      afterForLoopTimer = ktime_get_real();
      BitForLoopProcessingTime = ktime_to_ns(ktime_sub(afterForLoopTimer,startForLoopTimer));
      meanCalc(meanProccessingForLoopPtr, BitForLoopProcessingTime);
      if(forLoopCounter<DATA2COLLECT)
        ProccessingForLoopInterrarivalTime[forLoopCounter] = BitForLoopProcessingTime;
      forLoopCounter++;
      
      /* introduce Xnsec delay */
      //availableTimeSlot = (meanPacketPtr->mean) - mydiv(10*(meanPacketPtr->mean),100); /* DT - 10%DT */
      //timePerByte = mydiv(availableTimeSlot, numOfBytes); /* in usec */
      cpuDivisionDiff = timePerByte*1000 - BitForLoopProcessingTime;
        //cpuDivisionDiff = ((meanProccessingForLoopPtr->mean) - BitForLoopProcessingTime) + mydiv(byteCounter*(meanProccessingForLoopPtr->mean)*CPUFREQUENCY,REFERENCECPUFREQ) - ((long long int) (byteCounter*(meanProccessingForLoopPtr->mean)));
      if(cpuDivisionDiff>0)
        delayToIntroduce = ktime_to_ns(ktime_add_ns(ktime_get_real(),cpuDivisionDiff));
      else
        delayToIntroduce = ktime_to_ns(ktime_get_real());
      while(ktime_to_ns(ktime_get_real()) < delayToIntroduce);
          //printk(KERN_INFO "%lld , %lld \n", ktime_to_ns(ktime_get_real()), delayToIntroduce);
      /* ---------------------- */

      if(myByteFlag){
        /* introduce Xnsec delay */
        /*
        cpuDivisionDiff = mydiv(byteCounter*(meanProccessingForLoopPtr->mean)*CPUFREQUENCY,REFERENCECPUFREQ) - ((long long int) (byteCounter*(meanProccessingForLoopPtr->mean)));
        if(cpuDivisionDiff>0)
          delayToIntroduce = ktime_to_ns(ktime_add_ns(ktime_get_real(),cpuDivisionDiff));
        else
          delayToIntroduce = ktime_to_ns(ktime_get_real());
        while(ktime_to_ns(ktime_get_real()) < delayToIntroduce);
        */
        //raiseFlag();
        myByteFlagTimes[myByteFlagCounter] = BitForLoopProcessingTime;
        myByteFlag=false;
        myByteFlagCounter++;
      }
      /* ---------------------- */
      byteCounter++;
    }
    afterProccessingTime = ktime_get_real();
    if(packetCounter<DATA2COLLECT)
      ProccessingInterrarivalTime[packetCounter] = ktime_us_delta(afterProccessingTime,proccessingTime);
    packetCounter++;
    meanCalc(meanProccessingPtr, ktime_us_delta(afterProccessingTime,proccessingTime));
  }
  if(meanProccessingForLoopPtr)
    printk(KERN_INFO "Total number of For Loop Proccessing count = %zu ,providing a rolling average of %lld nseconds for Proccessing time.\n",meanProccessingForLoopPtr->samplesNum, meanProccessingForLoopPtr->mean);
  if(meanProccessingPtr)
    printk(KERN_INFO "Total number of Proccessing count = %zu ,providing a rolling average of %lld useconds for Proccessing time.\n",meanProccessingPtr->samplesNum, meanProccessingPtr->mean);  
  if(meanPacketPtr)
    printk(KERN_INFO "Total number of packets Received count = %zu ,providing a rolling average of %lld useconds for packet interarrival time.\n",meanPacketPtr->samplesNum, meanPacketPtr->mean);
  if(meanDataPtr)
    printk(KERN_INFO "Total number of valid Messages count = %zu ,providing a rolling average of %lld useconds for valid message arrival.\n",meanDataPtr->samplesNum, meanDataPtr->mean);
  if(meanSendMsgDataPtr)
    printk(KERN_INFO "Total number of sending Messages count = %zu ,providing a rolling average of %lld mseconds between messages of interest.\n",meanSendMsgDataPtr->samplesNum, meanSendMsgDataPtr->mean);
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
  HashData* test = NULL;

  if(byteOfMsg){
    for(i=0;i<70;i++)
      printk(KERN_INFO "| %hu | %lld\n", byteOfMsg[i], myByteFlagTimes[i]);
  }

  snprintf(logBuffer, 100, "| %zu | %lld | %zu | %lld | %lld | %zu | %lld | %zu | %lld\n", meanDataPtr->samplesNum, meanDataPtr->mean, meanSendMsgDataPtr->samplesNum, meanSendMsgDataPtr->mean, moduleTotalTime, meanPacketPtr->samplesNum, meanPacketPtr->mean, meanProccessingPtr->samplesNum, meanProccessingPtr->mean);
  kwrite(sendingfd, logBuffer, strlen(logBuffer));
  for(i=0;i<DATA2COLLECT;i++){ /* possible problems may arise here, no check for arbitary data in the dynamic array */
    snprintf(logBuffer, 100, "%lld | %lld | %lld\n", PacketInterrarivalTime[i], ProccessingInterrarivalTime[i], ProccessingForLoopInterrarivalTime[i]);
    kwrite(receivingfd, logBuffer, strlen(logBuffer));
  }


  for(i=0;i<successReceivedCounter;i++){
    snprintf(logBuffer, 100, "%hu | %hu | %hu | bitsPassedTilMsg: %zu\n", receivedMsgs[i], oldReceivedMsgs[i], afterReceivedMsgs[i], x_bits_til_next_valid_msg[i]);
    kwrite(receivingfd, logBuffer, strlen(logBuffer));
    if(x_bits_til_next_valid_msg[i]<=WAIT_X_BITS_FOR_MSG){
      if((test = searchValue(afterReceivedMsgs[i], hashMapArray))!=NULL)
        printk(KERN_INFO "%hu ", test->key);
    }
    else{
      if((test = searchValue(receivedMsgs[i], hashMapArray))!=NULL)
        printk(KERN_INFO "%hu ", test->key);
    }
  }
  for(i=0;i<successCounter;i++){
    snprintf(logBuffer, 100, "%hu\n", sentMsgs[i]);
    kwrite(sendingfd, logBuffer, strlen(logBuffer));
  }
  kclose(sendingfd);
  kclose(receivingfd);
  /* --------------------------------------------------------- */

  
  long long int moduleTotalTime = ktime_to_ms(ktime_sub(ktime_get_real(), moduleInTime)); /* equivalent to ktime_ms_delta(ktime_get_real(),moduleInTime) */
  printk(KERN_INFO "| %zu | %lld | %zu | %lld | %lld | %zu | %lld | %zu | %lld\n", meanDataPtr->samplesNum, meanDataPtr->mean, meanSendMsgDataPtr->samplesNum, meanSendMsgDataPtr->mean, moduleTotalTime, meanPacketPtr->samplesNum, meanPacketPtr->mean, meanProccessingPtr->samplesNum, meanProccessingPtr->mean);

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
