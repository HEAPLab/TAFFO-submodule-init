# TaffoClangPlugin User Guide
The Taffo framework needs some hints to get its job done. The programmer must then to place annotations about variables inside the code, otherwise TAFFO will not work. This plugin allows the C LLVM frontEnd (Clang, https://clang.llvm.org/) to parse these annotations in C programs and get them to the LLVM backend, so that TAFFO can find them in the Initialization pass.
In particular, these annotations consists of pragmas. Pragmas are a widely used feature to pass to the compiler some information about variables or how to execute some code and are part of the greater category of attributes. For a list of all supported attributes in clang, refer to https://clang.llvm.org/docs/AttributeReference.html.

TAFFO pragmas can be placed wherever in the code, each pragma on a separate line, whithout any other code on the same line. For a better readability, it's recommended to place a pragma just near the variable to be annotated.

This guide presents how to use the plugin, and the syntax and semantics of TAFFO pragmas.

## How to build and use the Plugin
 - (download and install LLVM 10 and Clang 10)
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
Taffo pragmas come with the following specific syntax:
 - S -> #pragma taffo VARNAME [FUNNAME] "ANNOTATION"
 - VARNAME -> STRING
 - FUNNAME -> STRING
 - STRING -> ([A-Z][a-z][0-9] [_])+
 - ANNOTATION follows the general TAFFO syntax for annotation specified in https://github.com/HEAPLab/TAFFO/blob/develop/doc/AnnotationSyntax.md
 
 When the syntax is not respected, a message is printed to std::out, and the annotation is ignored
 When there are more strings at the end of the pragma, a simple warning is generated, but the annotation remains valid, and the addit
 
 
## Semantics of TAFFO pragmas
 - VARNAME = name of the variable that's being annotated.
 - FUNNAME = name of the function where VARNAME is declared, and where VARNAME has its scope. Global variables are not declared inside any function: in this case no function name must be provided (that's why FUNNAME is optional).

 When the semantics of the annotation does not make sense(i.e. a mispelled varName or funName, or the function of varName is not the one declared in funName, etc...), unfortunately no warning is generated, and the pragma is ignored (so be careful!).
 
 

 

 




