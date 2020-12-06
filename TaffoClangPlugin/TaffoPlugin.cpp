//===- Attribute.cpp ------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which adds an an annotation to file-scope declarations
// with the 'example' attribute.
//
//===----------------------------------------------------------------------===//
//===- AnnotateFunctions.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which adds an annotation to every function in
// translation units that start with #pragma enable_annotate.
//
//===----------------------------------------------------------------------===//
#include "clang/AST/ASTContext.h"
#include "clang/Basic/PragmaKinds.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/StringSwitch.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Attr.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"


#include <iostream>
using namespace clang;


namespace {

struct PragmaTaffoInfo{
    std::string ID;
    std::string funName;
    std::string annotation;
  };

// 64 annotations is the maximum allowed for a compilation unit
static SmallVector<PragmaTaffoInfo, 64> InfoList;


class TaffoPragmaVisitor
  : public RecursiveASTVisitor<TaffoPragmaVisitor> {
public:
  explicit TaffoPragmaVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitVarDecl(VarDecl *Declaration) {
    // For debugging, dumping the AST nodes will show which nodes are being visited
    //Declaration->dump();
    //getting the name of the variable
    std::string Vname = Declaration->getQualifiedNameAsString();
    //getting the function name
    std::string Fname ="";
    auto FD = dyn_cast<FunctionDecl>(Declaration->getDeclContext());
    if (FD != NULL){
      //this is a local variable declaration
      Fname = FD->getNameInfo().getAsString();
    }
    //else for global declarations we keep the empty string as function name, which is coherent why how annotations are parsed
    
    //looking in the list if this variable has been annotated by the user
    for(PragmaTaffoInfo info : InfoList){
      if(info.ID.compare(Vname)==0 && info.funName.compare(Fname)==0){
        Declaration->addAttr(AnnotateAttr::CreateImplicit(Declaration->getASTContext(),
                                                 info.annotation));
      }
    }
    
    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *Declaration) {
    // For debugging, dumping the AST nodes will show which nodes are being visited
    // Declaration->dump();
    //getting the function name
    std::string Fname = Declaration->getQualifiedNameAsString();    
    //looking in the list if this variable has been annotated by the user
    for(PragmaTaffoInfo info : InfoList){
      if(info.ID.compare(Fname)==0 && info.funName.compare("")==0){
        Declaration->addAttr(AnnotateAttr::CreateImplicit(Declaration->getASTContext(),
                                                 info.annotation));
      }
    }
    
    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }

  private:
    ASTContext *Context;
};

//parsing phase
class TaffoPragmaConsumer : public ASTConsumer {
public:
  explicit TaffoPragmaConsumer(ASTContext *Context)
    : Visitor(Context) {}

  bool HandleTopLevelDecl(DeclGroupRef DG) override { 
    for (DeclGroupRef::iterator b = DG.begin(),e = DG.end(); b != e; ++b) {
      Visitor.TraverseDecl(*b);
    }
    return true;
  }
private:
  // A RecursiveASTVisitor implementation.
  TaffoPragmaVisitor Visitor;
  
};


class TaffoPragmaAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::unique_ptr<ASTConsumer>(new TaffoPragmaConsumer(&CI.getASTContext()));
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  PluginASTAction::ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};



// preprocessing phase
class TaffoPragmaHandler : public PragmaHandler {
public:
  TaffoPragmaHandler() : PragmaHandler("taffo") { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    Token Tok;
    //passing through the "taffo" string
    PP.Lex(Tok);

    ParseTaffoValue(PP, Tok);
}

  static void ParseTaffoValue(Preprocessor &PP, Token &Tok) {
    PragmaTaffoInfo Info;
    //parsing ID
    if (Tok.isNot(tok::identifier)) {
      std::cout << "Error, a Taffo pragma must contain an identifier\n";
      return;
    }
    IdentifierInfo *ID = Tok.getIdentifierInfo();
    Info.ID = ID->getName().str();
    PP.Lex(Tok);

    //parsing FunName (if its exists)
    if (Tok.isNot(tok::identifier)) {
      //this is an annotation for a global variable or for a function declaration, there is no funName
      Info.funName = "";
    }else{
      //this is an annotation for a local variable, funName has been specified
      IdentifierInfo *FunInfo = Tok.getIdentifierInfo();
      Info.funName = FunInfo->getName().str();
      PP.Lex(Tok);
    }

    if (Tok.is(tok::eod)) {
      std::cout << "Error,  a Taffo pragma must contain an annotation\n";
      return;
    }
    

    //parsing the actual annotation
    std::string annotation = "";
    std::string ann = Tok.getLiteralData();
    ann = ann.substr(0, ann.find("\n"));
    annotation += ann.substr(1, ann.find_last_of("\"")-1);
    PP.Lex(Tok);


    std::string extra = "";
    if (Tok.isNot(tok::eod)) {
      while (Tok.isNot(tok::eod)){
        std::string current = Tok.getLiteralData();
        std::cout << "extra token read: " << current << "\n";
        extra += current;
        PP.Lex(Tok);
      }
      std::cout << "Warning: extra tokens at the end of pragma taffo; " << extra << "----end of token\n";
      PP.Diag(Tok.getLocation(), diag::warn_pragma_extra_tokens_at_eol)
          << "taffo pragma";
    }


    //this is to deal with macro expansions
    size_t pos = 0;
    std::string search = "\"";
    std::string replace = "";
    while ((pos = annotation.find(search, pos)) != std::string::npos) {
         annotation.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    
    
    Info.annotation = annotation;
    InfoList.push_back(Info);
  } 
};

}

static FrontendPluginRegistry::Add<TaffoPragmaAction> X("taffo-plugin", "taffo plugin functions");
static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo","taffo pragma description");