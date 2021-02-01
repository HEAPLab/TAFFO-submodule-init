## TaffoClangPlugin Dev Guide aka What you need to know besides the User Guide
The plugin adds to Clang two components: a Pragma Handler (called creatively TaffoPragmaHandler) and a Frontend Plugin (TaffoPragmaAction), which is an extension of a Frontend Action. 

### Pragma Handler - Lexer phase [TaffoPlugin.ccp, HandlePragma function, line 122]
Pragma are actually just preprocessing directives. Thus when we want to add to the C language a pragma which carries some annotations  we need to declare a preprocessing handler for our custom pragma, and then find a way to pass the annotation to the backend (the LLVM IR generated code). The current way is to save all the annotations in an array, and then reinsert them as attributes.

The identifier of our pragma is the string "taffo". During the preprocessing phase all the pragmas that start with "taffo" are passed to the overridden HandlePragma function of our TaffoPragmaHandler class, which makes use of the auxiliary function ParseTaffoValue. The function works in the following way.

The current string of code is stored into Tok, while PP.Lex(Tok) is used to move to the next string of code. The function parses the pragma one string at a time, creates a TaffoPragmaInfo and adds it to a global vector. TaffoPragmaInfo is just a container for the parsed pragma. We use the double quotes for the actual annotation(s) to make all the strings which compose it parsed all together (what a trick!). 

They only purpose of allowing multiple annotations is actually to allow the use of macros inside a pragma: after a macro has been converted in the actual string, those double quotes are still present. Let's consider the example:

```c
#define ANNOTATION_RGBPIXEL         "struct[scalar(range(0,255)),scalar(range(0,255)),scalar(range(0,255)),void,scalar(range(0,1))]"
#define ANNOTATION_RGBIMAGE         "struct[void,void," ANNOTATION_RGBPIXEL "]"
#define SUB(x) _Pragma (#x)
#define DO_PRAGMA(x) SUB(x) 
DO_PRAGMA(taffo image main ANNOTATION_RGBIMAGE)
```


What gets to ParseTaffoPragma is (double quotes included):

```c
"struct[void,void," "struct[scalar(range(0,255)),scalar(range(0,255)),scalar(range(0,255)),void,scalar(range(0,1))]" "]" \n "struct[scalar(range(0,255)),scalar(range(0,255)),scalar(range(0,255)),void,scalar(range(0,1))]" "]" \n "]". 
```

We need then to parse the chopped annotation and throw away all the tokens after the first new line, taking care of ill-formed annotations where some characters are specified outside of double quotes.

Before moving to the actual annotation we check whether the same target has already been annotated: in case so, we emit a warning and stop the parsing. This forbids the insertion of multiple taffo pragmas for the same target.

When the preprocessing phase terminates, the pragma is thrown away and doesn't appear anymore in the code to compile.

Note that we need the programmer to specify the target id and funName because pragmas are not bounded to any other piece of code before or after the pragma itself, unlikely pure attributes.

EmitWarning is just a function to generate warnings for syntactically wrong pragmas in a modular way: before doing its job, it checks whether the warnings have been suppressed by the user.

### Frontend Plugin - Parsing phase [TaffoPlugin.cpp, createASTConsumer function, line 97]
A Frontend Action is an interface which allows the execution of custom actions over the Clang AST. For more information about the AST, please see the references. A Frontend Plugin is just a Frontend Action with an interesting feature: it allows for parsing command line options. We add TaffoPragmaAction to the FrontendPluginRegistry (line 238): our plugin will be run on the generated AST. The only useful command line option (at the moment) is **-Wno-ignored-pragmas**, which can be used to suppress all warnings about syntactically wrong pragmas. For a list of all currently emitted warnings, please refer to the next section. If you pass to the plugin any other string, this will be simply ignored.

The general purpose of the plugin is to look for all the variable and function declarations which have been annotated in the source code, and readd the annotations. We are looking then for VarDecl and FunctionDecl nodes in the AST, i.e. for nodes which correspond to variable and function declarations.

Practically, TaffoPragmaAction has to take care only to create the ASTConsumer (TaffoPragmaConsumer class, line 80), while executing it is left to the interface. The entry point for the ASTConsumer is HandleTopLevelDecl (line ), 82which is called on every top level declaration in the AST. 

Note that some easy to go plugin implementations provide handleTranslationUnit as entry point (e.g. PrintFunctionNames, in clang/examples). Unfortunately it's not suitable for our plugin since it would cause the ASTConsumer to be called after the generation of the entire AST, and at that time it's not possible anymore to modify the AST as we wish (all the modifications would have no effect, so frustrating!). I hope my time has not been wasted in vain.

### Warnings
| Warning message      | Meaning          | Example                   |
|:--------------------:|:----------------:|:--------------------------|
| "expected identifier" | The pragma does not have the id. | #pragma taffo |
| "already annotated target" | The target has already been annotated. | #pragma taffo var main "scalar()" \ #pragma taffo var main "scalar()" |
| "expected annotation" | The pragma does not have the annotation. | #pragma taffo var main |
| "annotation string outside double quotes" | The pragma has some string which belongs to the annotation part but it's outside double quotes. | #pragma taffo var main "scalar" () |
| "non matching double quotes" | The pragma has an odd number of double quotes: it's not possible to determine what's part of the annotations and what's not. | #pragma taffo var main "scalar" "() |




### Insertion of annotations [TaffoPlugin.cpp, VisitVarDecl and VisitFunctionDecl functions, lines 26 and 56]
TopLevelDecls aren't really our target (they are the Translation Unit declarations, there is one TopLevelDecl per Translation Unit), so we define a RecursiveASTVisitor (TaffoPragmaVisitor). We can make it traverse the entire AST through the function TraverseDecl. The TaffoPragmaVisitor class define a function VisitVarDecl, which is automatically called by the RecursiveASTVisitor interface on every matched VarDecl, and VisitFunctionDecl, which is called on every matched FunctionDecl. 

VisitVarDecl checks whether the declared variable has been annotated in the source code: if so, it adds the annotation as an attribute. VisitFunctionDecl does the same for functions declarations. Attributes are then handled by Clang through a call to the intrinsic function llvm.var.attributes (for local variables and function parameters), or to llvm.global.annotations (for global variables and function declarations). This puts the annotations exactly where the Taffo Initializer pass would expect that with the old syntax (__attribute(annotate("taffo_ann"))), so no change is required to the taffo framework.


### How to build the plugin alone ang generate LLVM IR to play with it
(clone and build LLVM 10 and Clang 10)
    
    - cd TaffoClangPlugin (wherever it is)
    
    - mkdir build
    
    - cd build
    
    - cmake ..
    
    - sudo make install
    
    - (navigate to the repo containing your C file enriched with #taffo pragmas, let's say main.c)
    
    - clang -c -Xclang -load -Xclang /usr/local/lib/TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin -S -emit-llvm main.c

This will generate LLVM IR enriched with annotations which can be parsed and used by TAFFO to enforce floating point to fixed point optimization.

If we want to generate LLVM IR and suppress warnings, type instead:

	- clang -c -Xclang -load -Xclang /usr/local/lib/TaffoPlugin.so -Xclang -add-plugin -Xclang taffo-plugin -Xclang -plugin-arg-taffo-plugin -Xclang -Wno-ignored-pragmas -S -emit-llvm main.c

### References (Clang 10)
[Clang AST tutorial](http://swtv.kaist.ac.kr/courses/cs453-fall13/Clang%20tutorial%20v4.pdf)

[Official Frontend Actions doc](https://releases.llvm.org/10.0.0/tools/clang/docs/RAVFrontendAction.html)

[Official Plugins doc](https://releases.llvm.org/10.0.0/tools/clang/docs/ClangPlugins.html)

[Plugin tutorial](https://chromium.googlesource.com/chromium/src/+/master/docs/writing_clang_plugins.md)

[Official Clang AST doc](https://releases.llvm.org/10.0.0/tools/clang/docs/IntroductionToTheClangAST.html)

[ASTConsumer code reference](https://clang.llvm.org/doxygen/classclang_1_1ASTConsumer.html)

[VarDecl code reference](https://clang.llvm.org/doxygen/classclang_1_1VarDecl.html#details)

[Stringification of macros](https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html) (this is for the gnu compiler, but it works the same way in clang)

 

 




