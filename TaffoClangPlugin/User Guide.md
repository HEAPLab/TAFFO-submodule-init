# TaffoClangPlugin User Guide
The Taffo framework needs some hints to get its job done. The programmer must then to place annotations inside the code, otherwise TAFFO will not work. This plugin allows the C LLVM frontEnd (Clang, https://clang.llvm.org/) to parse these annotations in C programs and get them to the LLVM backend, so that TAFFO can find them in the Initialization pass.
In particular, these annotations consists of pragmas. Pragmas are a widely used feature to pass to the compiler some information about how to compile a piece of code and are part of the greater category of attributes. For a list of all supported attributes in clang, please refer to https://clang.llvm.org/docs/AttributeReference.html.

TAFFO pragmas can be placed wherever in the code, each pragma on a separate line, whithout any other code on the same line. For better readability, it's recommended to place a pragma just near the piece of code to be annotated (here called "target").

Targets can be:
    
    - local variables
 
    - global variables
    
    - function parameters
    
    - function declarations
    
This guide presents how to use the plugin, and the syntax and semantics of TAFFO pragmas. For some example, see https://github.com/HEAPLab/TAFFO-test/tree/b770f61048a097da10f78ec3d328e8383c64da54

## How to build and use the Plugin
(download and install LLVM 10 and Clang 10)

    - git clone https://github.com/llvm/llvm-project 
 
    - cd llvm-project
    
    - git checkout release/10.x
    
    - mkdir build
    
    - cd build
    
    - cmake -DLLVM_ENABLE_PROJECTS=clang -G Ninja ..
 
    - sudo ninja install
 
    - cd ..
    
    - cd ..
    
    - git clone the current repo
    
    - cd current repo/current folder
    
    - mkdir build
    
    - cd build
    
    - cmake ..
    
    - sudo make install
    
    - (navigate to the repo containing your C file enriched with Taffo annotations, let's say main.c)
    
    - clang -c -Xclang -load -Xclang /usr/local/lib/TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin -S -emit-llvm main.c

This will generate LLVM IR enriched with annotations which can be parsed and used by TAFFO to enforce floating point to fixed point optimization.
 

## Syntax of TAFFO pragmas
Taffo pragmas follow a specific syntax depending on the pragma target:
 - S            -> #pragma taffo LOCALVAR|GLOBALVAR|FUNCTIONPAR|FUNCTIONDECL
 - LOCALVAR     -> ID FUNNAME "ANNOTATION"
 - GLOBALVAR    -> ID "ANNOTATION"
 - FUNCTIONPAR  -> ID FUNNAME "ANNOTATION"
 - FUNCTIONDECL -> ID
 - ID           -> STRING
 - FUNNAME      -> STRING
 - STRING       -> ([A-Z][a-z][0-9] [_])+
 - ANNOTATION follows the general TAFFO syntax for annotation specified in https://github.com/HEAPLab/TAFFO/blob/develop/doc/AnnotationSyntax.md
 
 When the syntax is not respected, a message is printed to std::out, and the annotation is ignored.
 
 When there are more strings at the end of the pragma, a simple warning is generated, but the annotation remains valid, and the additional strings are ignored.
 
 
## Semantics of TAFFO pragmas
 - ID      = name of the annotation target (it can be a variable identifier or a function name)
 - FUNNAME = name of the function where ID is declared, and where ID has its scope. When the target is a function declaration or a global variable, this field must not be specified (obviously they are not declared inside any other function)

 When the semantics of the annotation does not make sense(i.e. a mispelled id or funName, or the function of id is not the one declared in funName, etc...), unfortunately no warning is generated, and the pragma is ignored (so be careful!).

## Notes
Being a directive, the Taffo pragma cannot be produced as the result of macro expansion. 
To declare a pragma inside a macro, write (e.g. annotating variable id in main) _Pragma ("taffo id main  \"ANNOTATION\").

Likewise, to use a macro inside a pragma, we need the following workaround (we are annotating the variable image inside the main function): 
#define ANNOTATION_RGBPIXEL         "struct[scalar(range(0,255)),scalar(range(0,255)),scalar(range(0,255)),void,scalar(range(0,1))]"
#define ANNOTATION_RGBIMAGE         "struct[void,void," ANNOTATION_RGBPIXEL "]"
#define SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) SUB(x) 
DO_PRAGMA(taffo image main ANNOTATION_RGBIMAGE)



 

 

 




