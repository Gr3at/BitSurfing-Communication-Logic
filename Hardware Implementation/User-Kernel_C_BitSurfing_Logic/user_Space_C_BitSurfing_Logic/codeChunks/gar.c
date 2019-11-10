#include <unistd.h>
#include <stdio.h>
                                        //  / PC7 / PC6 / PC5 / PC4 / PC3 / PC2 / PC1 / PC0
#define  PC_CFG0_REG_DEFAULT 0x77777777 //0b0 111 0 111 0 111 0 111 0 111 0 111 0 111 0 111
                                      //  / PC31-PC19 PC18-PC0 PC6 / PC5 / PC4 / PC3 / PC2 / PC1 / PC0
#define PC_DATA_REG_DEFAUT 0x00000000 //  / 00000000 00000000 00000000 00000000

int main(void){
  printf("%d\n", PC_DEFAULT);
  return(0);
}

/*
XOR
0 0 --> 0
0 1 --> 1
1 0 --> 1
1 1 --> 0
*/
