#include <stdio.h>

int foo1(int, int);
int foo3(int, int);




int main() {
	
	int a;
	#pragma taffo target a "123" 
	#pragma taffo backtracking a "prova" 
	int b, c1, c2;
	a = 7;
	b = 5;
	int d = a;
	c1 = foo1(a, b);
	#pragma taffo target foo3 "ciao"
	c2 = foo3(a, b);
	printf("C1 is: %d\n", c1);
	printf("C2 is: %d\n", c2);
	return 0;
}


int foo1(int c, int b){
	int a = 1;
	int d = a;
	return c +b + a;
}

int foo3(int a, int b){
	return a-b;
}
