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

#define ANNOTATION_COMPLEX(R1,R2) "struct[scalar(" R1 "),scalar(" R2 ")]"
#define SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) SUB(x) 

#define TEST_ANNOTATION "test nested macro annotation"


#define gen_perform() {                                        \
    DO_PRAGMA(taffo sync foo1 "defined")                       \
    float sync = 0.0;                                            \
    DO_PRAGMA(taffo fooVar foo1 TEST_ANNOTATION)               \
    float fooVar;                                               \
}  





int main() {
	#pragma taffo a main "scalar(disabled range(-3000, 3000))" "extra tokensss"
	int a;
	int __attribute((annotate("scalar()"))) b, c1, c2;

	//array examples
	#pragma taffo array main "pragma array annotation"
	int array[5];

	int __attribute((annotate("annotate array annotation"))) vector[5];
	//////////////////////
	RgbPixel pixel;

	//clang push tests
	#pragma clang attribute push( __attribute((annotate("clang push annotation"))) , apply_to = variable)
	float push;
	float push_vector[5];
  	#pragma clang attribute pop

	a = 7;
	b = 5;
	int var;
	DO_PRAGMA(taffo var main "prefix")
	c1 = foo1(a, b);
	c2 = foo3(a, b);
	printf("C1 is: %d\n", c1);
	printf("C2 is: %d\n", c2);
	return 0;
}

#pragma taffo foo1 "scalar(disabled range(-4000, 4000))"
int foo1  (int c, int b) __attribute__((annotate("scalar(range(0, 256))"))) {
	int a = 1;
	int d = a;
	gen_perform()
	return c +b + a;
}

int foo3(int a, int b){
	return a-b;
}

