#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 2031

typedef struct Dataitem{
   unsigned short data;   
   unsigned short key;
}DataItem;

typedef struct Hasharray{
   size_t arrayLength;
   DataItem* dataPtr;
}HashArrayObj;

DataItem* hashArray[SIZE];
DataItem* item;

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

unsigned short hashCode(unsigned short);
void display(void);
void insert(unsigned short ,unsigned short);
DataItem *search(unsigned short);
DataItem *searchValue(unsigned short);
unsigned short msgInWord(unsigned short);
node_t * OpenAndStoreCodebookToVector(const char*);


#define DEFAULTMAPPINGKEYS 0  // used to indicate default mapping in hash map

unsigned short hashFunction(unsigned short key, size_t SIZE){
  return(key % SIZE);
}

HashArrayObj* createHashMap(int i ,node_t* cbPtr){
   size_t dummy = 0;
   DataItem* dataBPtr = NULL;
   HashArrayObj* HashArray = (HashArrayObj*)malloc(sizeof(HashArrayObj));
   HashArray->arrayLength = cbPtr->arrayLength;
   if((HashArray==NULL) || (cbPtr->arrayLength==0))
      return(NULL);
   while(dummy<cbPtr->arrayLength){
      if((dataBPtr = (DataItem*)malloc(sizeof(DataItem)))==NULL)
         return(NULL);
      dataBPtr[dummy].key=dummy;
      dataBPtr[dummy].data=cbPtr->arrayPtr[dummy];
      
      dummy++;
   }
   HashArray->dataPtr = dataBPtr;
   return(HashArray);
}

void displayhashes(HashArrayObj* hashMapArray){
   size_t i = 0;
   while(i<hashMapArray->arrayLength){
      printf(" (%d,%d)",hashMapArray->dataPtr[i].key,hashMapArray->dataPtr[i].data);
      i++;
   }
   printf("\n");
}

int main(){
   node_t *codebookPtr = NULL;
   if(((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt")) == NULL) || codebookPtr->arrayLength == 0){
      perror("Failed to store CodeBook to Vector.\n");
      exit(EXIT_FAILURE);
   }
   printf("%zu\n", codebookPtr->arrayLength);

   HashArrayObj* hashMapArray = createHashMap((int)DEFAULTMAPPINGKEYS ,codebookPtr);
   displayhashes(hashMapArray);

   for(unsigned short i=0;i<codebookPtr->arrayLength; i++)
      insert(i,codebookPtr->arrayPtr[i]);

   item = search(2030);
   if(item != NULL)
      printf("Element found: %d\n", item->data);
   else
      printf("Element not found\n");

   item = searchValue(60854);
   if(item != NULL)
      printf("Element found: %hu %d\n", item->key, item->data);
   else
      printf("Element not found\n");

   /* Open and process DataInput file */
   unsigned short filebuffer = 0;
   FILE *fileptr = fopen("testInput.txt", "rb");
   fseek(fileptr, 0, SEEK_END);
   size_t fileBytesLen = ftell(fileptr);
   printf("file size : %zu bytes\n", fileBytesLen);
   rewind(fileptr);

   unsigned char* buffer = (unsigned char *)malloc((fileBytesLen+1)*sizeof(unsigned char)); // Enough memory for file + \0
   fread(buffer, fileBytesLen, 1, fileptr);
   fclose(fileptr);

   unsigned short msgFound = 0;
   int i = 0;
   while(i<fileBytesLen){
      filebuffer = (buffer[i] << 8) + buffer[i+1];   //0100100001100101 - 18533
      if((msgFound=msgInWord(filebuffer))!=0){
         //do stuff with the message (like send it....)
         i+=2; // since the message was found by hashing 2 bytes we don't need to check next byte
      }
      else{ // if msgFound==0 which means there is no match from hashCode for 2-byte msg
         msgFound=msgInWord(buffer[i]);
         //do stuff with the message
         i++;
      }
   }
   /* ------------------------------- */

   //display();
if(msgInWord(0b0000011111101110)!=0)
   printf("%hu\n", msgInWord(0b0000011111101110));

return(0);
}

unsigned short hashCode(unsigned short key){
   return(key % SIZE);
}

DataItem *search(unsigned short key){
   if(key>=SIZE)
      return(NULL);  // this holds in case ascending valued keys are used
   unsigned short hashIndex = hashCode(key); //get the hash
   //move in array until an empty
   while(hashArray[hashIndex] != NULL){
      //printf("%d %d\n", hashIndex, hashArray[hashIndex]->data);
      if(hashArray[hashIndex]->key == key)
         return hashArray[hashIndex]; 
      ++hashIndex;
      hashIndex %= SIZE;   //wrap around the table
   }

   return(NULL);
}

DataItem *searchValue(unsigned short value){
   for(int i = 0; i<SIZE; i++){
      if((hashArray[i] != NULL) && (hashArray[i]->data==value))
         return(hashArray[i]);
   }

   return(NULL);
}

void insert(unsigned short key,unsigned short data){
   DataItem *item = (DataItem*) malloc(sizeof(DataItem));
   item->data = data;  
   item->key = key;

   unsigned short hashIndex = hashCode(key);

   while(hashArray[hashIndex]!=NULL){
      ++hashIndex;
      hashIndex %= SIZE;   //wrap around the table
   }

   hashArray[hashIndex] = item;
}

void display(){
   for(int i=0; i<SIZE; i++){
      if(hashArray[i] != NULL)
         printf(" (%d,%d)",hashArray[i]->key,hashArray[i]->data);
      else
         printf(" ~~ ");
   }
   printf("\n");
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

node_t * OpenAndStoreCodebookToVector(const char *filename){
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

unsigned short msgInWord(unsigned short word){
   item = search(word);
   if((item != NULL) && (item->key == word))
      return(item->data);  //msgToSend = item->data;
   return(0);
}
