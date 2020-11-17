# Pragma Notes

## About the repo
The submodule llvm_project, in the branch release/10.x, has been modified to handle custom taffo pragmas in clang with LLVM 10.
Here a non complete list of modified files(some headers files may be missing):
 - clang/lib/ParsePragma.cpp
 - include/clang/Sema/TaffoHint.h
 - lib/Parse/ParseStmt.cpp
 - include/clang/Basic/Attr.td
 - lib/Sema/SemaStmtAttr.cpp
 - lib/CodeGen/CGS.cpp
 
## General notes about the implementation
 - the pragma can be placed in the code only AFTER the declaration of the target variable
 - all pragmas are added to single llvm instructions as metadata


## (Currently implemented) Syntax of Taffo pragmas
 - S -> #pragma taffo OPTION "VARNAME" "OPTIONARG"
 - OPTION -> target | backtracking
 - VARNAME -> STRING
 - OPTIONARG -> STRING
 - STRING -> ([A-Z][a-z][0-9] [_])+

## Semantics of Taffo pragmas
 - Varname = the identifier of the variable to be annotated
 - OptionArg = a parameter of the specified option. Its semantics may vary depending on the option. A syntx valid string here may not have a meaning with the corresponding option, thus leading to a non valid pragma statement.

## Clang plugin ideas
 - keep the same syntax as the current annotations
 - parse them and add as attributes to varDecl AST nodes, to avoid modifying the current Taffo Init implementation 

## To do
 - write a clang plugin for taffo pragmas to use instead of a custom version of clang
 - write a syntax for Fortran taffo pragmas which cannot be transformed into attributes
 - parse them correctly in taffo init module.
 - implement pragma handling in taffo.