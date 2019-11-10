/* 
The goal here is to implement BeagleBoneBlack bitreceiver.

The code below is intented to:
1)Setup properly BBB's input and output pins
2)Setup a UDP socket
3)Load the provided codebook of valid messages to an array of unsigned shorts (word size = 2Bytes)
4)Choose a random message from the codebook to send (msgToSent)
5)Setup sending and receiving log files
6)(while) loop over and over until the node has 100 successful sent messages
in the loop	a) receive a byte
			b) add byte to buffer
			c) if a signal from input gpio is detected : search buffer for valid message and log result
			d) else check if message of interest (msgToSent) is in buffer, if true send signal to RasberryPi, log message, choose a new message to send and increment success counter
7) terminate
*/
#include <stdio.h>      /* for getline(), fopen(), printf() and fprintf() */
#include <stdlib.h>     /* for exit() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for clock() */
#include <stdbool.h>    /* for bool, only in C99 or alternatively : typedef enum {false, true} bool; */
#include <fcntl.h>      /* for file descriptors control */
#include <sys/mman.h>   /* for mmap() */
#include <sys/time.h>
#include <sched.h>      /* to prevent swapping */

#define MAXRECVSTRING 1  /* Longest string to receive */
#define ELAPSEDTIMEINSECONDS ((double)(time(0)-startTime))
#define SUCCESSES_TO_FINISH 20
#define PREFIX 0b1110

/* Valid only for BeagleBoneBlack with AM335x ARM Cortex-A8 Microprocessor */
#define GPIO2_BASE_ADDR 0x481AC000
#define GPIO2BANK_SIZEINBYTES 4096
#define OE_ADDR 0x134
#define GPIO_DATAOUT 0x13C
#define GPIO_DATAIN 0x138
#define GPIO2_2_RELATIVE_POSITION 2 // board pin P8.7
#define GPIO2_5_RELATIVE_POSITION 5 // board pin P8.9
/* ----------------------------------------------------------------------- */

typedef struct codeBookData{
    size_t arrayLength;
    unsigned short *arrayPtr;
}node_t;

long currentTimeMillis(){
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

node_t * OpenAndStoreCodebookToVector(char*);
unsigned short pickRandomMsgToSend(node_t*);
unsigned short searchValidMsgInBuffer(unsigned int, node_t*);
bool msgToSentInBuffer(unsigned short, unsigned int);
bool gpioEventDetected(volatile unsigned long*);
void raiseFlag(volatile unsigned long*);

int main(void)
{
    /* Assign Highest possible priority */
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &sp);
    mlockall(MCL_CURRENT | MCL_FUTURE);
    /* -------------------------------- */
    time_t startTime=time(0), tempTime=time(0);
    int sock;
    struct sockaddr_in broadcastAddr;
    unsigned short broadcastPort = 37020;
    unsigned char receivedByte[MAXRECVSTRING];
    int receivedByteLen;
    unsigned int buffer = 0;    /* Node Buffer of int(=32bit) size */

    srand(time(NULL));

    /* Setup Input and Output Pins */
    int mem_fd;
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
        perror("Unable to access \"/dev/mem\"\n Are you root?\n");
        exit(EXIT_FAILURE);
    }
    volatile unsigned long *map_base = (volatile unsigned long*) mmap(NULL, GPIO2BANK_SIZEINBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO2_BASE_ADDR);
    close(mem_fd);
    if (((void*)map_base) == MAP_FAILED){
        perror("mmap failed.\n Are you root?\n");
        exit(EXIT_FAILURE);
    }
    map_base[OE_ADDR/4] &= (0xFFFFFFFF ^ (1 << GPIO2_2_RELATIVE_POSITION)); // setup output pin
    map_base[OE_ADDR/4] |= (1 << GPIO2_5_RELATIVE_POSITION);    // setup input pin
    /* --------------------------- */
    
    /* -------- Setup UDP Socket -------- */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("socket() failed\n");
        exit(EXIT_FAILURE);
    }
    
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    broadcastAddr.sin_port = htons(broadcastPort);
    
    if(bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0){
        perror("socket bind() failed\n");
        exit(EXIT_FAILURE);
    }
    /* ---------------------------------- */
    
    /* Load CodeBook and randomly choose a message of interest */
    node_t *codebookPtr = NULL;
    if((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL){
        perror("Failed to store CodeBook to Vector.\n");
        exit(EXIT_FAILURE);
    }
    unsigned short msgToSent = pickRandomMsgToSend(codebookPtr);
    /* ------------------------------------------------------- */

    //long start_usec;
    unsigned short foundMsg = 0;
    unsigned short successCounter = 0;
    unsigned short successReceivedCounter = 0;
    unsigned short receivedMsgs[SUCCESSES_TO_FINISH];
    unsigned short oldReceivedMsgs[SUCCESSES_TO_FINISH];
    unsigned short afterReceivedMsgs[SUCCESSES_TO_FINISH];
    bool cacheNextMsg = false;
    unsigned int   foundMsgsCache = 0;
    unsigned short sentMsgs[SUCCESSES_TO_FINISH];
    //FILE* allMsgsfd = fopen("logMsgs.txt", "a");

    printf("Ready to receive...\n");
    while(successCounter<SUCCESSES_TO_FINISH){
        //start_usec = currentTimeMillis();
        if((receivedByteLen = recvfrom(sock, receivedByte, MAXRECVSTRING, 0, NULL, 0)) < 0){
            perror("recvfrom() failed. Terminating...\n");
            break;
        }

        buffer = (buffer << 8) + receivedByte[0];

        if((foundMsg = searchValidMsgInBuffer(buffer, codebookPtr))!=0){
            foundMsgsCache = (foundMsgsCache << 16) | foundMsg;
            if(cacheNextMsg){
                    afterReceivedMsgs[successReceivedCounter-1]=foundMsg;
                    cacheNextMsg=false;
                }
            //fprintf(allMsgsfd, "%hu | %ld\n", foundMsg, currentTimeMillis() - start_usec);
        }

        if(gpioEventDetected(map_base)){
            receivedMsgs[successReceivedCounter] = foundMsgsCache;
            oldReceivedMsgs[successReceivedCounter] = (foundMsgsCache >> 16);
            successReceivedCounter++;
            cacheNextMsg=true;
        }
        else{
            if(msgToSentInBuffer(msgToSent, buffer)){
                raiseFlag(map_base);
                sentMsgs[successCounter] = msgToSent;
                msgToSent = pickRandomMsgToSend(codebookPtr);
                successCounter++;
                if(difftime(time(0), tempTime) >= 1800){
                    printf("Sent : %hu | Received : %hu\n", successCounter,successReceivedCounter);
                    tempTime = time(0);
                }
            }
        }
    }

    /* Open both sending and receiving log files in append mode. */
    FILE* sendingfd = fopen("sendBBB.txt", "a");
    FILE* receivingfd = fopen("recBBB.txt", "a");
    if((sendingfd==NULL) || (receivingfd)==NULL){
        perror("Couldn't open log Files in append mode.\n");
        exit(EXIT_FAILURE);
    }
    /* --------------------------------------------------------- */
    for(int i=0;i<successReceivedCounter;i++)
    	fprintf(receivingfd, "%hu | %hu | %hu\n", receivedMsgs[i], oldReceivedMsgs[i], afterReceivedMsgs[i]);
    for(int i=0;i<successCounter;i++)
    	fprintf(sendingfd, "%hu\n", sentMsgs[i]);

    //fclose(allMsgsfd);
    fclose(sendingfd);
    fclose(receivingfd);
    munmap((void*)map_base, GPIO2BANK_SIZEINBYTES);
    close(sock);
    printf("NodeReceiver took %f seconds to finish.\n", ELAPSEDTIMEINSECONDS);
    exit(EXIT_SUCCESS);
}

unsigned short binaryString2short(char* s){
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
    char *buff = NULL;
    size_t buffLen = 0;
    ssize_t lineLen;
    size_t fileLines = 0;
    size_t dummy = 0;
    unsigned short *cbPtr = NULL;
    node_t* codeBPtr = (node_t*)malloc(sizeof(node_t));

    FILE *fin = fopen(filename, "r");
    if(fin != NULL){
        while((lineLen = getline(&buff, &buffLen, fin)) != -1)
            fileLines++;

        fseek(fin, 0, SEEK_SET);
        cbPtr = (unsigned short *)malloc(fileLines*sizeof(unsigned short));
        if(cbPtr==NULL){
            perror("Couldn't allocate memmory\n");
            exit(EXIT_FAILURE);
        }
        
        while(((lineLen = getline(&buff, &buffLen, fin)) != -1) && (dummy < fileLines))
            cbPtr[dummy++] = binaryString2short(buff);
        
        free(buff);
        fclose(fin);
        codeBPtr->arrayPtr = cbPtr;
        codeBPtr->arrayLength = fileLines;
    }
    return(codeBPtr);
}

unsigned short pickRandomMsgToSend(node_t *cBPtr){
    size_t randomIndex = rand() % cBPtr->arrayLength + 0;
    return(cBPtr->arrayPtr[randomIndex]);
}

int cmpfunc(const void * a, const void * b){
    return (*(unsigned short*)a - *(unsigned short*)b);
}

unsigned short searchValidMsgInBuffer(unsigned int buff, node_t* codeBPtr){
    unsigned short tempBuffer = 0;
    unsigned short *foundMsgPtr = NULL;

    for(int i=7;i>=0;i--){
        tempBuffer = buff >> i;
        if(!(PREFIX^(tempBuffer>>12 /* 16-12=4bits prefix left to check for equality */))){
            foundMsgPtr = (unsigned short*)bsearch(&tempBuffer, codeBPtr->arrayPtr, codeBPtr->arrayLength, sizeof(unsigned short), cmpfunc);
            if(foundMsgPtr != NULL)
                return(*foundMsgPtr);
        }
    }
    return(0); //zero indicates that no valid message was found (considering that zero is't a valid message itself)
}

/* for old implementation which was inside if(gpioEventDetected())

unsigned short searchValidMsgInBuffer(unsigned int buff, node_t* codeBPtr){
    unsigned short tempBuffer = 0;
    unsigned short *foundMsgPtr = NULL;

    for(int i=(8*sizeof(tempBuffer));i>=0;i--){
        tempBuffer = buff >> i;
        if(!(PREFIX^(tempBuffer>>12))){
            foundMsgPtr = (unsigned short*)bsearch(&tempBuffer, codeBPtr->arrayPtr, codeBPtr->arrayLength, sizeof(unsigned short), cmpfunc);
            if(foundMsgPtr != NULL)
                return(*foundMsgPtr);
        }
    }
    return(0); //zero indicates that no valid message was found (considering that zero is't a valid message itself)
}
*/

bool msgToSentInBuffer(unsigned short msgToSent, unsigned int buff){
    unsigned short tmpSubBuffer = 0;
    for(int i=(8*sizeof(msgToSent));i>=0;i--){
        tmpSubBuffer = buff >> i;
        if(!(msgToSent^tmpSubBuffer))
            return(true);
    }
    return(false);
}

bool gpioEventDetected(volatile unsigned long *mapBaseAddr){
    static size_t eventCounter = 0;
    if(((eventCounter%2==0) && (mapBaseAddr[GPIO_DATAIN/4] & (1 << GPIO2_5_RELATIVE_POSITION))) 
        || ((eventCounter%2!=0) && (!(mapBaseAddr[GPIO_DATAIN/4] & (1 << GPIO2_5_RELATIVE_POSITION)))))
    {
        eventCounter++;
        return(true);
    }
    return(false);
}

void raiseFlag(volatile unsigned long *mapBaseAddress){
    static size_t raiseCounter = 0;
    if(raiseCounter%2==0)
        mapBaseAddress[GPIO_DATAOUT/4]  |= (1 << GPIO2_2_RELATIVE_POSITION);  // set output to HIGH
    else
        mapBaseAddress[GPIO_DATAOUT/4]  ^= (1 << GPIO2_2_RELATIVE_POSITION);  // set output to LOW
    raiseCounter++;
}
