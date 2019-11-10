/* 
The goal here is to implement BeagleBoneBlack bitreceiver.

The code below is intented to:
1)Setup properly BBB's input and output pins
2)Setup a UDP socket
3)Load the provided codebook of valid messages to an array of unsigned shorts (word size = 2Bytes)
4)Choose a random message from the codebook to send (msgToSent)
5)Setup sending and receiving log files
6)(while) loop over and over until the node has SUCCESSES_TO_FINISH successful sent messages
in the loop a) receive a byte
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
#define SUCCESSES_TO_FINISH 5
#define PREFIX 0b1110

typedef struct codeBookData{
    size_t arrayLength;
    unsigned short *arrayPtr;
}node_t;

node_t * OpenAndStoreCodebookToVector(char*);
unsigned short pickRandomMsgToSend(node_t*);
unsigned short searchValidMsgInBuffer(unsigned int);
bool msgToSentInBuffer(unsigned short, unsigned int);

int main(void){
    /* Assign maximum priority */
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &sp);
    mlockall(MCL_CURRENT | MCL_FUTURE);
    /* ----------------------- */
    time_t startTime = time(0), tempTime=time(0);
    int sock;
    struct sockaddr_in broadcastAddr;
    unsigned short broadcastPort = 37020;
    unsigned char receivedByte[MAXRECVSTRING];
    int receivedByteLen;
    unsigned int buffer = 0;    /* Node Buffer of int(=32bit) size */

    srand(time(NULL));

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
    
    /* Load CodeBook and randomly choose a message to send*/
    node_t *codebookPtr = NULL;
    if(((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL) || codebookPtr->arrayLength == 0){
        perror("Failed to store CodeBook to Vector.\n");
        exit(EXIT_FAILURE);
    }
    unsigned short msgToSent = pickRandomMsgToSend(codebookPtr);
    /* -------------------------------------------------- */

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

    printf("Ready to receive...\n");
    while((successCounter<SUCCESSES_TO_FINISH) && (successReceivedCounter<SUCCESSES_TO_FINISH)){
        if((receivedByteLen = recvfrom(sock, receivedByte, MAXRECVSTRING, 0, NULL, 0)) < 0){
            perror("recvfrom() failed. Terminating...\n");
            break;
        }

        buffer = (buffer << 8) + receivedByte[0];

        for (int bits = 7; bits >= 0; bits--){
            x_bits_till_next_msg++;
            if((foundMsg = searchValidMsgInBuffer(buffer>>bits))!=0){
                foundMsgsCache = (foundMsgsCache << 16) + foundMsg;
                if(cacheNextMsg){
                    afterReceivedMsgs[successReceivedCounter-1]=foundMsg;
                    cacheNextMsg=false;
                    x_bits_til_next_valid_msg[successReceivedCounter-1]=x_bits_till_next_msg;
                }
            }
            if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
                    sentMsgs[successCounter] = msgToSent;
                    msgToSent = pickRandomMsgToSend(codebookPtr);
                    successCounter++;
                    if(difftime(time(0), tempTime) >= 1800){
                    printf("Sent : %hu | Received : %hu\n", successCounter, successReceivedCounter);
                    tempTime = time(0);
                    }
            }
        }
    }

    /* Open both sending and receiving log files in append mode. */
    FILE* sendingfd = fopen("send.txt", "a");
    FILE* receivingfd = fopen("rec.txt", "a");
    if((sendingfd==NULL) || (receivingfd)==NULL){
        perror("Couldn't open log Files in append mode.\n");
        exit(EXIT_FAILURE);
    }
    /* --------------------------------------------------------- */
    for(int i=0;i<successReceivedCounter;i++)
        fprintf(receivingfd, "%hu | %hu | %hu | bitsPassedTilMsg: %zu\n", receivedMsgs[i], oldReceivedMsgs[i], afterReceivedMsgs[i], x_bits_til_next_valid_msg[i]);
    for(int i=0;i<successCounter;i++)
        fprintf(sendingfd, "%hu\n", sentMsgs[i]);

    fclose(sendingfd);
    fclose(receivingfd);
    close(sock);
    printf("NodeReceiver took %f seconds to finish.\n", ELAPSEDTIMEINSECONDS);
    exit(EXIT_SUCCESS);
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
            cbPtr[dummy++] = binary2short(buff);
        
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
