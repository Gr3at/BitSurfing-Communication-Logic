/* 
The goal here is to implement Raspberry bitreceiver.

The code below is intented to:
1)Setup properly RPi's input and output pins
2)Setup a UDP socket
3)Load the provided codebook of valid messages to an array of unsigned shorts (word size = 2Bytes)
4)Choose a random message from the codebook to send (msgToSent)
5)Setup sending and receiving log files
6)(while) loop over and over until the node has 100 successful sent messages
in the loop a) receive a byte
            b) add byte to buffer
            c) if a signal from input gpio is detected : search buffer for valid message and log result
            d) else check if message of interest (msgToSent) is in buffer, if true send signal to BBB, log message, choose a new message to send and increment success counter
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
#include <wiringPi.h>   /* for GPIO manipulation */
#include <sys/time.h>
#include <sys/resource.h>   /* for setpriority() */

#define MAXRECVSTRING 1  /* Longest message to receive */
#define ELAPSEDTIMEINSECONDS ((double)(time(0)-startTime))
#define SUCCESSES_TO_FINISH 50
#define PREFIX 0b1110

/* WiringPi Pins numbers in BCM_GPIO format */
#define INPIN 17    /* board pin 11 */
#define OUTPIN 27   /* board pin 13 */
/* ---------------------------------------- */

typedef struct codeBookData{
    size_t arrayLength;
    unsigned short *arrayPtr;
}node_t;

node_t * OpenAndStoreCodebookToVector(char*);
unsigned short pickRandomMsgToSend(node_t*);
unsigned short searchValidMsgInBuffer(unsigned int, node_t*);
bool msgToSentInBuffer(unsigned short, unsigned int);
bool gpioEventDetected(void);
void raiseFlag(void);

int main(void)
{
    setpriority(PRIO_PROCESS, 0, -20); /* request maximum priority */
    time_t startTime=time(0); //, tempTime=time(0);
    int sock;
    struct sockaddr_in broadcastAddr;
    unsigned short broadcastPort = 37020;
    unsigned char receivedByte[MAXRECVSTRING];
    int receivedByteLen;
    unsigned int buffer = 0;    /* Node Buffer of size int(=32bit) */

    srand(time(NULL));

    /* Setup Input and Output Pins */
    if(wiringPiSetupGpio() == -1){
        perror("Failed to initialize wiringPi.\n Are you root?\n");
        exit(EXIT_FAILURE);
    }
    pinMode(OUTPIN, OUTPUT);
    pinMode(INPIN, INPUT);
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

    unsigned short foundMsg = 0;
    unsigned short successCounter = 0;
    unsigned short successReceivedCounter = 0;
    unsigned short receivedMsgs[SUCCESSES_TO_FINISH];
    unsigned short oldReceivedMsgs[SUCCESSES_TO_FINISH];
    unsigned int   foundMsgsCache = 0;
    unsigned short sentMsgs[SUCCESSES_TO_FINISH];
    
    while(successCounter<SUCCESSES_TO_FINISH){
        if((receivedByteLen = recvfrom(sock, receivedByte, MAXRECVSTRING, 0, NULL, 0)) < 0){
            perror("recvfrom() failed. Terminating...\n");
            break;
        }

        buffer = (buffer << 8) + receivedByte[0];

        for (int bits = 7; bits >= 0; bits--){
            if((foundMsg = searchValidMsgInBuffer((buffer>>bits), codebookPtr))!=0)
                foundMsgsCache = (foundMsgsCache << 16) + foundMsg;

            if(gpioEventDetected()){
                receivedMsgs[successReceivedCounter] = foundMsgsCache;
                oldReceivedMsgs[successReceivedCounter] = (foundMsgsCache >> 16);
                successReceivedCounter++;
            }
            else{
                if(msgToSentInBuffer(msgToSent, (buffer>>bits))){
                    raiseFlag();
                    sentMsgs[successCounter] = msgToSent;
                    msgToSent = pickRandomMsgToSend(codebookPtr);
                    successCounter++;
                }
            }
        }
    }

    /* Open both sending and receiving log files in append mode. */
    FILE* sendingfd = fopen("sendLog.txt", "a");
    FILE* receivingfd = fopen("recLog.txt", "a");
    if((sendingfd==NULL) || (receivingfd)==NULL){
        perror("Couldn't open log Files in append mode.\n");
        exit(EXIT_FAILURE);
    }
    /* --------------------------------------------------------- */
    for(int i=0;i<successReceivedCounter;i++)
        fprintf(receivingfd, "%hu | %hu \n", receivedMsgs[i], oldReceivedMsgs[i]);
    for(int i=0;i<successCounter;i++)
        fprintf(sendingfd, "%hu\n", sentMsgs[i]);

    fclose(sendingfd);
    fclose(receivingfd);
    close(sock);
    printf("NodeReceiver took %f seconds to finish\n", ELAPSEDTIMEINSECONDS);
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
    unsigned short tempBuffer = buff;
    unsigned short *foundMsgPtr = NULL;

    if(!(PREFIX^(tempBuffer>>12 /* 16-12=4bits prefix left to check for equality */))){
        foundMsgPtr = (unsigned short*)bsearch(&tempBuffer, codeBPtr->arrayPtr, codeBPtr->arrayLength, sizeof(unsigned short), cmpfunc);
        if(foundMsgPtr != NULL)
            return(*foundMsgPtr);
        }
    return(0); //zero indicates that no valid message was found (considering that zero is't a valid message itself)
}

bool msgToSentInBuffer(unsigned short msgToSent, unsigned int buff){
    unsigned short tmpSubBuffer = buff;
    if(!(msgToSent^tmpSubBuffer))
        return(true);
    return(false);
}

bool gpioEventDetected(void){
    static size_t eventCounter = 0;
    if(((eventCounter%2==0) && (digitalRead(INPIN)==HIGH)) || ((eventCounter%2!=0) && (digitalRead(INPIN)==LOW))){
        eventCounter++;
        return(true);
    }
    return(false);
}

void raiseFlag(void){
    static size_t raiseCounter = 0;
    if(raiseCounter%2==0)
        digitalWrite(OUTPIN, HIGH);
    else
        digitalWrite(OUTPIN, LOW);
    raiseCounter++;
}
