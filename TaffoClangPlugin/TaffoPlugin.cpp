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
    std::string varName;
    std::string funName;
    std::string annotation;
  };


static SmallVector<PragmaTaffoInfo, 32> InfoList;


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
      if(info.varName.compare(Vname)==0 && info.funName.compare(Fname)==0){
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
    //parsing through the "taffo" string
    PP.Lex(Tok);

    if (!ParseTaffoValue(PP, Tok)){
     return;
    }
    
    if (Tok.isNot(tok::eod)) {
      std::cout << "Error,  unexpected extra tokens at the end of pragma taffo\n";
      PP.Diag(Tok.getLocation(), diag::warn_pragma_extra_tokens_at_eol)
          << "taffo pragma";
      return;
    }

}

  static bool ParseTaffoValue(Preprocessor &PP, Token &Tok) {
    PragmaTaffoInfo Info;
    //parsing VarName
    if (Tok.isNot(tok::identifier)) {
      std::cout << "Error, a Taffo pragma must contain a variable identifier\n";
      return false;
    }
    IdentifierInfo *VarInfo = Tok.getIdentifierInfo();
    Info.varName = VarInfo->getName().str();
    PP.Lex(Tok);

    //parsing FunName (if its exists)
    if (Tok.isNot(tok::identifier)) {
      //this is an annotation for a global variable, there is no funName
      Info.funName = "";
    }else{
      //this is an annotation for a local variable, funName has been specified
      IdentifierInfo *FunInfo = Tok.getIdentifierInfo();
      Info.funName = FunInfo->getName().str();
      PP.Lex(Tok);
    }

    if (Tok.is(tok::eod)) {
      std::cout << "Error,  a Taffo pragma must contain an annotation\n";
      return false;
    }
    

    //parsing the actual annotation
    std::string annotation = "";
    annotation = Tok.getLiteralData();
    annotation = annotation.substr(0, annotation.find("\n"));
    annotation = annotation.substr(1, annotation.size()- 2);
    PP.Lex(Tok);
    
    
    Info.annotation = annotation;
    InfoList.push_back(Info);
    return true;
  } 
};

}

static FrontendPluginRegistry::Add<TaffoPragmaAction> X("taffo-plugin", "taffo plugin functions");
static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo","taffo pragma description");