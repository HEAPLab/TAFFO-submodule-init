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
#include "clang/Tooling/Tooling.h"
#include <iostream>

using namespace clang;


namespace {

struct PragmaTaffoInfo{
    Token PragmaName;
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
    // For debugging, dumping the AST nodes will show which nodes are already
    // being visited.
    Declaration->dump();
    //getting the name o the variable
    std::string Vname = Declaration->getQualifiedNameAsString();
    std::cout << "Identifier " << Vname << "\n";
    //getting the function name, if the variable was declared inside a function
    std::string Fname ="";
    auto FD = dyn_cast<FunctionDecl>(Declaration->getDeclContext());
    if (FD != NULL){
      Fname = FD->getNameInfo().getAsString();
      std::cout << "the variable was declared in function " << Fname << "\n";
    }

    for(PragmaTaffoInfo info : InfoList){
      if(info.varName.compare(Vname)==0 && info.funName.compare(Fname)==0){
        std::cout << "Found correspondance on var " << info.varName << "\n";
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


class TaffoPragmaConsumer : public ASTConsumer {
public:
  explicit TaffoPragmaConsumer(ASTContext *Context)
    : Visitor(Context) {}
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
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
    Token PragmaName = Tok;
    SmallVector<Token, 1> TokenList;
    
    PP.Lex(Tok);
    if (Tok.isNot(tok::identifier)) {
      printf("Error, a Taffo pragma must contain at least an option argument and a variable identifier\n");
     return;
    }
    if (!ParseTaffoValue(PP, Tok, PragmaName))
     return;



  if (Tok.isNot(tok::eod)) {
    printf("Error,  unexpected extra tokens at the end of pragma taffo\n");
    PP.Diag(Tok.getLocation(), diag::warn_pragma_extra_tokens_at_eol)
        << "taffo pragma";
    return;
  }

}

  static bool ParseTaffoValue(Preprocessor &PP, Token &Tok,Token PragmaName) {
    std::string annotation = "";
    PragmaTaffoInfo Info;
    //parsing VarName
    IdentifierInfo *VarInfo = Tok.getIdentifierInfo();
    Info.varName = VarInfo->getName().str();
    PP.Lex(Tok);

    //parsing FunName
    IdentifierInfo *FunInfo = Tok.getIdentifierInfo();
    Info.funName = FunInfo->getName().str();
    PP.Lex(Tok);

    //parsing the actual annotation
    while (Tok.isNot(tok::eod)) {
      IdentifierInfo *OptionInfo = Tok.getIdentifierInfo();
      annotation = annotation + " " + OptionInfo->getName().str();
      PP.Lex(Tok);
    }
    
    Info.annotation = annotation;
    Info.PragmaName = PragmaName;
    InfoList.push_back(Info);
    return true;
  } 
};

}


static FrontendPluginRegistry::Add<TaffoPragmaAction> X("taffo-plugin", "taffo plugin functions");
static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo","taffo pragma description");