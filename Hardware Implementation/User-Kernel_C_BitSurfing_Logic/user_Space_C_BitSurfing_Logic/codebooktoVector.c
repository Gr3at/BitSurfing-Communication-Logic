#define _GNU_SOURCE
#include <stdio.h>      /* for getline(), fopen() */
#include <stdlib.h>     /* exit() */
#include <sys/time.h>

#define TIME_IN_MILLISEC (double)((tv.tv_sec)*1000 + (tv.tv_usec)/1000)

int cmpfunc(const void * a, const void * b){
	return ( *(unsigned short*)a - *(unsigned short*)b );
}

unsigned short searchValidMsgInBuffer(unsigned int buff, unsigned short * codeBPtr){
    unsigned short tempBuffer = 0;
    unsigned short *foundMsgPtr = NULL;

    for(int i=(8*sizeof(tempBuffer));i>=0;i--){
        tempBuffer = buff >> i;
    
        foundMsgPtr = (unsigned short*)bsearch(&tempBuffer, codeBPtr, 2031, sizeof(unsigned short), cmpfunc);
        if(foundMsgPtr != NULL){
            printf("foundMsgPtr Value = %hu\n", *foundMsgPtr);
            return(*foundMsgPtr);
        }
    }
    return(0); //zero indicates that no valid message was found (considering that zero is't a valid message itself)
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

int main(){
	char *buffer = NULL;
	size_t buffLen = 0;
	ssize_t lineLen;
	size_t fileLines = 0;
	size_t dummy = 0;
	unsigned short *codebookPtr = NULL;

    FILE *fin = fopen("codebook1.txt", "r");
    if(fin != NULL){
    	while((lineLen = getline(&buffer, &buffLen, fin)) != -1)
    		fileLines++;

    	fseek(fin, 0, SEEK_SET);
    	codebookPtr = (unsigned short *)malloc(fileLines*sizeof(unsigned short));
    	if(codebookPtr==NULL){
    		perror("Couldn't allocate memmory\n");
    		exit(EXIT_FAILURE);
    	}
    	while(((lineLen = getline(&buffer, &buffLen, fin)) != -1) && (dummy < fileLines)){
    		codebookPtr[dummy] = binary2short(buffer);
    		dummy++;
    	}
    	for(int i=0;i<fileLines;i++)
			printf("num is : %d\n", codebookPtr[i]);

		free(buffer);
		fclose(fin);
	}
	else
		exit(EXIT_FAILURE);

	/* Test searchValidMsgInBuffer Functions */
	unsigned int buffr = 2342543234;
	unsigned short foundMsg = searchValidMsgInBuffer(buffr, codebookPtr);
	printf("Found Msg : %hu\n", foundMsg);
	/* ------------------------------------- */

	/* Test time in milliseconds */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	printf("%.0f milliseconds\n", TIME_IN_MILLISEC);
	/* ------------------------- */

	/* Test msgOfInterestInBuffer implementation */
	unsigned short msgToSend = foundMsg;
	for(int i=(8*sizeof(msgToSend));i>=0;i--){
		unsigned short tempMsg = buffr >> i;
		printf("%hu\t%hu\n", msgToSend, tempMsg);
		if(!(msgToSend^tempMsg))
			printf("Great\n");        
    }
	/* ----------------------------------------- */

	exit(EXIT_SUCCESS);
}
