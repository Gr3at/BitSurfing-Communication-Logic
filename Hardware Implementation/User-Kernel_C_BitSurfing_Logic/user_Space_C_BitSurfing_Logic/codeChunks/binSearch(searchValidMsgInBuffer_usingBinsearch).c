#include <stdio.h>
#include <stdlib.h>


int cmpfunc(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int values[] = { 5, 20, 29, 32, 63 };

int main () {
   int *item;
   int key = 312;

   /* using bsearch() to find value 32 in the array */
   item = (int*) bsearch (&key, values, 5, sizeof (int), cmpfunc);
   if( item != NULL ) {
      printf("Found item = %d\n", *item);
   } else {
      printf("Item = %d could not be found\n", key);
   }
   
   unsigned short temp1 = 0;
   unsigned int buff = 23423433;

   temp1 = buff >> 9;
   printf("short = int %d\n", temp1);


   return(0);
}


/*
unsigned short searchValidMsgInBuffer(unsigned int buff, node_t* codeBPtr){
    unsigned short tempBuffer = buff;
    unsigned short *foundMsgPtr = NULL;

    if(!(PREFIX^(tempBuffer>>12))){
        foundMsgPtr = (unsigned short*)bsearch(&tempBuffer, codeBPtr->arrayPtr, codeBPtr->arrayLength, sizeof(unsigned short), cmpfunc);
        if(foundMsgPtr != NULL)
            return(*foundMsgPtr);
        }
    return(0); //zero indicates that no valid message was found (considering that zero is't a valid message itself)
}
*/
