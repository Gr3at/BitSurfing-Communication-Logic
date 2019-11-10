#include <stdio.h>
#include <stdlib.h>

typedef unsigned char BYTE;

int main(){
	BYTE random_number;
	
	while(1){
		random_number = rand() % 256 + 0; // rand() % range + min
		if(random_number == 0b00000001)
			printf("Sucess %d\n", random_number);
	}
	
	return(0);
}