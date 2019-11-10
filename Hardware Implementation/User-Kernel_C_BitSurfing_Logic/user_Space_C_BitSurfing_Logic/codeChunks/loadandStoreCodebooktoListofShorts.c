#define _GNU_SOURCE
#include <stdio.h>      /* for getline(), fopen() */
#include <stdlib.h>     /* exit() */

typedef struct node {
    unsigned short val;
    struct node * next;
} node_t;

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
	node_t *head = NULL, *tempPtr = NULL;

    FILE *fin = fopen("codebook1.txt", "r");
    if(fin != NULL){
    	while((lineLen = getline(&buffer, &buffLen, fin)) != -1){
			//printf("Retrieved line of length %zu : \n", lineLen);
			printf("You typed: '%s' which is '%d'\n",buffer, binary2short(buffer));
			if(head == NULL){
				if((tempPtr = (node_t *)malloc(sizeof(node_t))) == NULL){
					perror("Couldn't allocate memmory\n");
					exit(EXIT_FAILURE);
				}
				tempPtr->val = binary2short(buffer);
				tempPtr->next = NULL;
				head = tempPtr;
			}
			else{
				if((tempPtr->next = (node_t *)malloc(sizeof(node_t))) == NULL){
					perror("Couldn't allocate memmory\n");
        			exit(EXIT_FAILURE);
				}
				tempPtr = tempPtr->next;
				tempPtr->val = binary2short(buffer);
				tempPtr->next = NULL;
			}
		}
    }
    /* // uncomment if you want to count number of list elements or print values
    tempPtr = head;
    size_t counter = 0;
    while(tempPtr!=NULL){
    	counter++;
    	tempPtr = tempPtr->next;
    }
    printf("%zu\n", counter);
	*/

	free(buffer);
	fclose(fin);
	exit(EXIT_SUCCESS);
}
