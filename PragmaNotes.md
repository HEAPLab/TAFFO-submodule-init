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

## (Currently implemented) Syntax of Taffo pragmas
S -> #pragma taffo OPTION "VARNAME" "OPTIONARG"
OPTION -> target | backtracking
VARNAME -> ID
OPTIONARG -> ID
ID -> ([A-Z][a-z][0-9] [_])*

## Semantics of Taffo pragmas
Varname = the identifier of the variable to be annotated
OptionArg = a parameter of the specified option. Its semantics may vary depending on the option. A syntx valid string here may not have a meaning with the corresponding option, thus leading to a non valid pragma statement.


## To do
 - parse them correctly in taffo init module.
 - extend clang to support all the remaining taffo annotations.
 - implement the same pragmas in flang.