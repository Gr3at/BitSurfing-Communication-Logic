//#define _GNU_SOURCE
#include <stdio.h>      /* for getline(), fopen(), printf() and fprintf() */
#include <stdlib.h>     /* for exit() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for clock() */
#include <sys/time.h>   /* for struct timeval */
#include <stdbool.h>    /* for bool, only in C99 or alternatively : typedef enum {false, true} bool; */
#include <sys/time.h>
#include <sys/resource.h>   /* for setpriority() */

#define MAXRECVSTRING 1  /* Longest string to receive */
#define ELAPSEDTIME ((double)(clock()-t))/CLOCKS_PER_SEC
#define ELAPSEDTIMEINSECONDS ((double)(time(0)-startTime))
#define TIME_IN_MILLISEC (double)((tv.tv_sec)*1000 + (tv.tv_usec)/1000)
#define PREFIX 0b1110

typedef struct node{
    size_t arrayLength;
    unsigned short *arrayPtr;
}node_t;

node_t * OpenAndStoreCodebookToVector(char*);
unsigned short pickRandomMsgToSend(node_t*);
unsigned short searchValidMsgInBuffer(unsigned int, node_t*);
bool msgToSentInBuffer(unsigned short, unsigned int);

int main()
{
    setpriority(PRIO_PROCESS, 0, -20); /* request maximum priority */
    time_t startTime=time(0); // tempTime=time(0);
    struct timeval tv;
    clock_t t = clock();
    int sock;
    struct sockaddr_in broadcastAddr;
    unsigned short broadcastPort = 37020;
    unsigned char receivedByte[MAXRECVSTRING];
    int receivedByteLen;
    unsigned int buffer = 0;    /* Node Buffer of int(=32bit) size */

    //printf("unsigned int size : %lu | unsigned short size : %lu\n", sizeof(unsigned int), sizeof(unsigned short));
    
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
    
    /* Load CodeBook and randomly choose a message of interest */
    node_t *codebookPtr = NULL;
    if((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL){
        perror("Failed to store CodeBook to Vector.\n");
        exit(EXIT_FAILURE);
    }
    unsigned short msgToSent = pickRandomMsgToSend(codebookPtr);
    /* ------------------------------------------------------- */

    /* Open both sending and receiving log files in append mode. */
    FILE* sendingfd = fopen("sendLog.txt", "a");
    FILE* receivingfd = fopen("recLog.txt", "a");
    if((sendingfd==NULL) || (receivingfd)==NULL){
        perror("Couldn't open log Files in append mode.\n");
        exit(EXIT_FAILURE);
    }
    /* --------------------------------------------------------- */

    //HERE
    unsigned short foundMsg = 0;
    unsigned short successCounter = 0;

    while(successCounter<100){
        if((receivedByteLen = recvfrom(sock, receivedByte, MAXRECVSTRING, 0, NULL, 0)) < 0){
            perror("recvfrom() failed. Terminating...\n");
            break;
        }

        buffer = (buffer << 8) + receivedByte[0];

        /* delete 4 lines below */
        foundMsg = searchValidMsgInBuffer(buffer, codebookPtr);
        msgToSentInBuffer(msgToSent, buffer);
        msgToSent = pickRandomMsgToSend(codebookPtr);
        successCounter++;
        /* -------------------- */
        /*
        if(gpioEventDetected()){
            foundMsg = searchValidMsgInBuffer(buffer, codebookPtr);
            gettimeofday(&tv, NULL);
            fprintf(receivingfd, "%hu | %.0f\n", foundMsg, TIME_IN_MILLISEC);
        }
        else
            if(msgToSentInBuffer(msgToSent, buffer)){
                raiseFlag();
                gettimeofday(&tv, NULL);
                fprintf(sendingfd, "%hu | %.0f\n", msgToSent, TIME_IN_MILLISEC);
                randomIndex = rand() % codebookPtr->arrayLength + 0;
                msgToSent = codebookPtr->arrayPtr[randomIndex];
                successCounter++;
            }
        */
        //printf("Received: %d\n", (int)receivedByte[0]);
    }

    fclose(sendingfd);
    fclose(receivingfd);
    close(sock);
    printf("NodeReceiver took ELAPSEDTIMEINSECONDS : %f seconds to finish \n", ELAPSEDTIMEINSECONDS);
    printf("NodeReceiver took difftime : %f seconds to finish \n", difftime(time(0), startTime));
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
        while(((lineLen = getline(&buff, &buffLen, fin)) != -1) && (dummy < fileLines)){
            cbPtr[dummy] = binary2short(buff);
            dummy++;
        }
        
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

bool msgToSentInBuffer(unsigned short msgToSent, unsigned int buff){
    unsigned short tmpSubBuffer = 0;
    for(int i=(8*sizeof(msgToSent));i>=0;i--){
        tmpSubBuffer = buff >> i;
        if(!(msgToSent^tmpSubBuffer))
            return(true);
    }
    return(false);
}
/*
bool gpioEventDetected(void){
    return(false);
}

void raiseFlag(void){
}
*/