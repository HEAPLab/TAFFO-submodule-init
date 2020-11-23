# TaffoPlugin notes

## How to use th taffo plugin
 - (download and install LLVM 10,Clang 10 and TAFFO from their official Github repositories)
 - mkdir build
 - cd build
 - cmake ..
 - make
 - cd ..
 (supposing you have the file main.c in the current folder)
 - clang -c -Xclang -load -Xclang ./build/TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin main.c
 - To generate LLVM IR, type clang -c -Xclang -load -Xclang ./build/TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin -S -emit-llvm main.c
 

## Syntax of taffo pragma
 - #pragma taffo VARNAME FUNNAME "ANNOTATION"
 - VARNAME -> STRING
 - FUNNAME -> STRING
 - STRING -> ([A-Z][a-z][0-9] [_])+
 - ANNOTATION follows the general taffo syntax for annotation specified in https://github.com/HEAPLab/TAFFO/blob/develop/doc/AnnotationSyntax.md
 
## Semantics of taffo pragma
 - VARNAME = name of the variable it's being annotated.
 - FUNNAME = name of the function where VARNAME is declared.
 
## TODO
 - Find a solution for global variables which do no live in any function
 




