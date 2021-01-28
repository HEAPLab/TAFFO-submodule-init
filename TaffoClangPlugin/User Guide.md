## TaffoClangPlugin User Guide
The Taffo framework needs some hints to get its job done. The programmer must then place some annotations inside the code, otherwise TAFFO will not work. This plugin allows the C LLVM frontEnd (Clang, https://clang.llvm.org/) to parse these annotations in C programs and get them to the LLVM backend, so that TAFFO can find them in the Initialization pass.
In particular, these annotations are wrapped inside pragmas. Pragmas are a widely used feature to pass to the compiler some information about how to compile a piece of code and are part of the greater category of attributes. For a list of all supported attributes in clang, please refer to https://clang.llvm.org/docs/AttributeReference.html.

TAFFO pragmas can be placed wherever in the code, each pragma on a separate line, whithout any other code on the same line. For better readability, it's recommended to place a pragma just near the piece of code to be annotated (here called "target").

Targets are:

    - local variables
 
    - global variables
    
    - function parameters
    
    - function declarations
    
This guide presents the syntax and semantics of TAFFO pragmas, as well as some examples. To get further information, please refer to the Dev Guide.

### Syntax of TAFFO pragmas
Taffo pragmas follow a specific syntax depending on the pragma target (non terminals are indicated with capital letters):

    - S            -> #pragma taffo LOCALVAR|GLOBALVAR|FUNCTIONPAR|FUNCTIONDECL  
    - LOCALVAR     -> ID FUNNAME ("ANNOTATION")+
    - GLOBALVAR    -> ID ("ANNOTATION")+
    - FUNCTIONPAR  -> ID FUNNAME ("ANNOTATION")+
    - FUNCTIONDECL -> ID ("ANNOTATION")+
    - ID           -> STRING
    - FUNNAME      -> STRING
    - STRING       -> ([A-Z][a-z][0-9] [_])+
    
[ANNOTATION](https://github.com/HEAPLab/TAFFO/blob/develop/doc/AnnotationSyntax.md) follows the general TAFFO syntax for annotation specified in the link.
 
When the syntax is not respected, a warning is generated, and the annotation is ignored. So a wrong formatted syntax does not lead to a rejection of Clang to compile the code (as it may be expected). This complies with the general behaviour of pragmas.
 
More than one annotation may be specified in double quotes: they are simply parsed together, as if they were a unique annotation. As a special case, the pragma is accepted even if there is no space character between two annotations.

Just one pragma per target can be written: when there are more than one, just the first is considered valid and accepted, whilst the second one will be ignored, and a warning is generated.
 
### Semantics of TAFFO pragmas

    - ID      = name of the annotation target (it can be a variable identifier or a function name)
    - FUNNAME = name of the function where ID is declared, and where ID has its scope. 
                When the target is a function declaration or a global variable, 
                this field must not be specified (obviously they are not declared inside any other function)

 When the semantics of the annotation does not make sense (i.e. a mispelled id or funName, or the function of id is not the one declared in funName, etc...), unfortunately no warning is generated, and the pragma is ignored (so be careful!).

### Notes
Being a directive, the Taffo pragma cannot be produced as the result of macro expansion (because macro expansion are preprocessing directives as well). To declare a pragma inside a macro, write (e.g. annotating variable id in main):

```c
 _Pragma ("taffo id main  \"example_annotation\"").
```

Likewise, to use a macro ( or some macros) inside a pragma, we need the following workaround (let's say we are annotating the variable image inside the main function with a macro string):

```c
#define ANNOTATION_RGBPIXEL         "struct[scalar(range(0,255)),scalar(range(0,255)),scalar(range(0,255)),void,scalar(range(0,1))]"
#define ANNOTATION_RGBIMAGE         "struct[void,void," ANNOTATION_RGBPIXEL "]"
#define SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) SUB(x) 
DO_PRAGMA(taffo image main ANNOTATION_RGBIMAGE)
```

The DO_PRAGMA workaround works also in the first case, i.e. declaring a pragma inside a macro, and it's the only way to declare, inside a macro, a pragma which in turn uses a macro inside itself.

### Example code
Here you can find an example code from the tests with some pragmas used in different ways and with different targets.

```c
#include <stdio.h>

int foo(int, int);

#define ANNOTATION(R1,R2) "scalar(range(" R1 "," R2 ) final)"
#define SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) SUB(x) 
\\ these are valid examples of pragmas inside a macro
\\ and actually the second one is an example of a macro inside a pragma
#define macro() {                                               \
    _Pragma("taffo macro_a foo \"scalar()\"")                   \
    float macro_a = 2.1423;                                     \
    DO_PRAGMA(taffo fooVar foo ANNOTATION(-3000,3000))          \
    float macro_b = 0.36547;                                    \
    printf("macro_a: %f\n", macro_a);                           \
    printf("macro_b: %f\n", macro_b);                           \
}  
#define MAX_N (30)
int main(int argc, char *argv[])
{
  #pragma taffo number main "scalar()"  \\ this annotation is accepted(no warning is generated)
                                        \\ but will not lead to any annotation:
                                        \\ number is defined in foo , not in main
  #pragma taffo numbers main "scalar()" \\ this is a valid local variable pragma, and it's the first one for this variable
                                        \\ this pragma is syntactically and semantically correct
  float numbers[MAX_N];
  int n = 0;
  #pragma taffo tmp main "scalar(disabled range(-3000, 3000))" \\this is a valid local variable pragma
  #pragma taffo tmp main "scalar()" \\ this is the second pragma of tmp: this annotation will be ignored
  float tmp;
  for (int i=0; i<MAX_N; i++) {
    if (scanf("%f", &tmp) < 1)
      break;
    numbers[n++] = tmp;
  }
  #pragma taffo add main "scalar" "()" \\ this is a valid local variable pragma
  float add = 0.0;
  #pragma taffo sub main "scalar""()" \\ this is valid too!
  #pragma taffo sub foo "scalar()" \\ this is valid too, but be careful: you are annotating the variable inside foo function!
  float sub = 0.0;
  DO_PRAGMA(taffo div main ANNOTATION(-3000,3000)) \\ an example of a macro inside a local variable pragma definition
  float div = 1.0;
  #pragma taffo mul main "scalar" "(")" \\this is not valid, and will be ignored
  #pragma taffo mul main "scalar()"\\this is valid
  float mul = 1.0;
  for (int i=0; i<n; i++) {
    add += numbers[i];
    sub -= numbers[i];
    if (numbers[i] != 0.0)
      div /= numbers[i];
    mul *= numbers[i];
  }

  printf("add: %f\nsub: %f\ndiv: %f\nmul: %f\n", add, sub, div, mul);
  
  int b = foo(add);
  printf("b: %f\n", b);
  
  return 0;
}

#pragma taffo foo "scalar()" \\ this is a valid function declaration pragma
int foo(float number){
  #pragma taffo number foo "scalar()" \\ this is a valid function parameter pragma
  
  int sub = 0.0;
  float tmp = number + 1.273646 - sub;
  macro();
  return tmp;
}
```


 

 

 




