#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Hashdata{
   unsigned short data;   
   unsigned short key;
}HashData;

typedef struct Hasharray{
   size_t arrayLength;
   HashData **dataPtr;
}HashArrayObj;

typedef struct codeBookData{
  size_t arrayLength;
  unsigned short *arrayPtr;
}node_t;

unsigned short hashFunction(unsigned short key, size_t SIZE){
  return(key % SIZE);
}

void addItemToArray(unsigned short key, unsigned short data, HashArrayObj* hArray){
   unsigned short hindex = hashFunction(key, hArray->arrayLength);
   HashData* newItem = (HashData*)malloc(sizeof(HashData));
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
  HashArrayObj* HashArray = (HashArrayObj*)malloc(sizeof(HashArrayObj));
  HashArray->arrayLength = cbPtr->arrayLength;
  HashArray->dataPtr = (HashData**)malloc((HashArray->arrayLength)*sizeof(HashData*));
  if((HashArray==NULL) || (HashArray->arrayLength==0) || (HashArray->dataPtr==NULL))
    return(NULL);
  for(size_t dummy=0; dummy<HashArray->arrayLength; dummy++)
    HashArray->dataPtr[dummy] = NULL;
  /* -------------------- */

  /* fill hash array with key-data pairs */
  for(unsigned short i=0; i<256; i++)
    addItemToArray(i, cbPtr->arrayPtr[i], HashArray);

  for(unsigned short i=256; i<HashArray->arrayLength-1; i++)
    addItemToArray(wHPtr->arrayPtr[i-256], cbPtr->arrayPtr[i], HashArray);
  /* ----------------------------------- */
  return(HashArray);
}

void displayhashes(HashArrayObj* hashMapArray){
   size_t i = 0;
   while(i<hashMapArray->arrayLength){
    if(hashMapArray->dataPtr[i]==NULL)
      printf(" (NULL,NULL)");
    else
      printf(" (%d,%d)",hashMapArray->dataPtr[i]->key,hashMapArray->dataPtr[i]->data);
    i++;
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

node_t * OpenAndStoreCodebookToVector(const char *filename, unsigned char bin2short){
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
    if(bin2short==1){
      while(((lineLen = getline(&buff, &buffLen, fin)) != -1) && (dummy < fileLines))
        cbPtr[dummy++] = binary2short(buff);
    }
    else{
      while(((lineLen = getline(&buff, &buffLen, fin)) != -1) && (dummy < fileLines))
        cbPtr[dummy++] = (buff[0] << 8) + buff[1];
    }

   free(buff);
   fclose(fin);
   codeBPtr->arrayPtr = cbPtr;
   codeBPtr->arrayLength = fileLines;
   }
   return(codeBPtr);
}

HashData *search(unsigned short key, HashArrayObj* hashArray){
  unsigned short hIndex = hashFunction(key, hashArray->arrayLength);
  while(hashArray->dataPtr[hIndex] != NULL){
    if(hashArray->dataPtr[hIndex]->key == key)
      return(hashArray->dataPtr[hIndex]); 
    hIndex++;
    hIndex %= hashArray->arrayLength;   //wrap around the table
  }

  return(NULL);
}

HashData *searchValue(unsigned short value, HashArrayObj* hashArray){
   for(int i = 0; i<hashArray->arrayLength; i++){
      if((hashArray->dataPtr[i] != NULL) && (hashArray->dataPtr[i]->data==value))
         return(hashArray->dataPtr[i]);
   }
   return(NULL);
}

unsigned short msgInWord(unsigned short word, HashArrayObj* hashArray){
   HashData* item = search(word, hashArray);
   if((item != NULL) && (item->key == word))
      return(item->data);  //msgToSend = item->data;
   return(0);
}

int main(){
   node_t *codebookPtr = NULL, *wordHashesPtr = NULL;
   if(((codebookPtr = OpenAndStoreCodebookToVector("codebook1.txt", 1)) == NULL) || codebookPtr->arrayLength == 0){
      perror("Failed to store CodeBook to Vector.\n");
      exit(EXIT_FAILURE);
   }
   
   printf("%zu\n", codebookPtr->arrayLength);

  if(((wordHashesPtr = OpenAndStoreCodebookToVector("freqs.txt", 0)) == NULL) || codebookPtr->arrayLength == 0){
    perror("Failed to store CodeBook to Vector.\n");
    exit(EXIT_FAILURE);
  }
  printf("wordHashesPtr length : %zu\n", wordHashesPtr->arrayLength);

  HashArrayObj* hashMapArray = createAndFillHashArray(codebookPtr, wordHashesPtr);

   displayhashes(hashMapArray);
   HashData * test = searchValue(61439, hashMapArray);
   if(test!=NULL)
    printf("\n\n(%hu, %hu)\n", test->key, test->data);
  else
    printf("\n\n(NULL, NULL)\n");
   printf("\n\nfound %hu\n" , msgInWord(0b01111010, hashMapArray)); //0b1110111111111111
   printf("%hu\n", 0b0000001111101110); 


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
   int c = 0;
   while(buffer[c]!='\0'){
      printf("%c", buffer[c]);
      c++;
   }
   printf("%d\n", c);
   return(0);
}