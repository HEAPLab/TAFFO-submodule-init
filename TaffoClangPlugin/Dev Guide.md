# TaffoClangPlugin Dev Guide aka What you need to know besides the User guide
The plugin adds to Clang two components: a Pragma Handler ( called creatively TaffoPragmaHandler) and a Frontend Plugin (TaffoPragmaAction), which is an extension of a Frontend Action. 

## Pragma Handler - Lexer phase
Pragma are actually just preprocessing directives. Thus when we want to add a pragma to the C language we need to declare an handler for our custom pragma, and then find a way to pass the annotation to the backend. The current way is to save all the annotation in an array, and then reinsert them as attributes.

The identifier of our pragma is the string "taffo". During the preprocessing phase of the code all the pragmas that start with "taffo" are passed to the overridden HandlePragma function of our TaffoPragmaHandler, which works in the following way.

The current string of code is stored into Tok, while PP.Lex(Tok) is used to move to the next string of code. ParseTaffoValue is just a function which parses the pragma one string at a time, creates a TaffoPragmaInfo and adds it to a global array. TaffoPragmaInfo is just a container for the parsed pragma. We use the double quotes for the actual annotation to make it parsed all together(what a trick!). Processing stops at the end of line. the pragma is then thrown away and doesn't appear anymore in the code.

We need the programmer to specify the varName and funName because pragmas are not bounded to any other piece of code before or after the pragma itself, unlikely pure attributes.

## Frontend Plugin - Parsing phase
A Frontend Action is an interface which allows the execution of custom actions over the Clang AST. For more information about the AST, please see the references. A PFrontend Plugin is just a Frontend Action with an interesting feature: it allows for parsing command line options. Despite we don't need any command line option in Taffo at the moment, we'll leave the door open for the future: the function ParseArgs simply returns true. 

The purpose of the plugin is to look for all the variable declaration which have been annotated in the source code, and readd the annotations. We are looking then for VarDecl nodes in the AST, i.e. for nodes which correspond to variable decalarations.

Practically, TaffoPragmaAction has to take care only to create and implement the ASTConsumer, while executing it is left to the interface. The entry point for the ASTConsumer is HandleTopLevelDecl, which is called on every top level declaration in the AST. 

Note that some easy to go plugin implementations provide handleTranslationUnit as entry point (e.g. PrintFunctionNames, in clang/examples). Unfortunately it's not suitable for our plugin since it would cause the ASTConsumer to be called after the generation of the entire AST, and at that time it's not possible anymore to modify the AST as we wish (all the modifications would have no effect, so frustrating!).

Anyway, TopLevelDecls aren't our target (they are the Translation Unit declarations, there is one TopLevelDecl per Translation Unit), so we provide a RecursiveASTVisitor (TaffoPragmaVisitor), which traverses the entire AST, and define a function VisitVarDecl, which is called on every matched VarDecl. 

VisitVarDecl checks whether the declared variable has been annotated: if so, it adds the annotation as an attribute. Attributes are then handled by Clang through a call to intrinsic function llvm.var.attributes (for local variables and function parameters), or to llvm.global.annotations (for global variables and function declarations). This puts the annotations exactly where the Taffo Initialized pass would expect that with the old syntax (__attribute(annotate("taffo_ann"))), so no change is required to the taffo framework.

## References(Clang 10)
[Clang AST tutorial](http://swtv.kaist.ac.kr/courses/cs453-fall13/Clang%20tutorial%20v4.pdf)

[Official Frontend Actions doc](https://releases.llvm.org/10.0.0/tools/clang/docs/RAVFrontendAction.html)

[Official Plugins doc](https://releases.llvm.org/10.0.0/tools/clang/docs/ClangPlugins.html)

[Plugin tutorial](https://chromium.googlesource.com/chromium/src/+/master/docs/writing_clang_plugins.md)

[Official Clang AST doc](https://releases.llvm.org/10.0.0/tools/clang/docs/IntroductionToTheClangAST.html)

[ASTConsumer code reference](https://clang.llvm.org/doxygen/classclang_1_1ASTConsumer.html)

[VarDecl code reference](https://clang.llvm.org/doxygen/classclang_1_1VarDecl.html#details)

 

 




