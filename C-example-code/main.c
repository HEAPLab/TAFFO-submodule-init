#include <stdio.h>

int foo1(int, int);
int foo3(int, int);

typedef struct {
   float r;
   float g;
   float b;
   int cluster;
   float distance;
} RgbPixel;




int main() {
	
	int a __attribute((annotate("scalar(disabled range(-3000, 3000))")));
	#pragma taffo target a "123" 
	#pragma taffo backtracking a "prova" 
	int b, c1, c2;
	RgbPixel pixel;
	#pragma taffo errtarget a "taffo"
	#pragma taffo scalar a

	a = 7;
	b = 5;
	int d = a;
	c1 = foo1(a, b);
	c2 = foo3(a, b);
	printf("C1 is: %d\n", c1);
	printf("C2 is: %d\n", c2);
	return 0;
}

#pragma enable_annotate
int foo1(int c, int b){
	int a = 1;
	int d = a;
	return c +b + a;
}

int foo3(int a, int b){
	return a-b;
}
