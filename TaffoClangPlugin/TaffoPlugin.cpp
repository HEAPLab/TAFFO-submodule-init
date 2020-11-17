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
using namespace clang;

namespace {

struct PragmaTaffoInfo{
    Token PragmaName;
    llvm::ArrayRef<std::string> annotation;
  };



class TaffoClassConsumer : public ASTConsumer {
public:
  explicit TaffoClassConsumer(ASTContext *Context)
    : Visitor(Context) {}
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    HandledDecl = true;

    for (auto D : DG)
      if (FunctionDecl *FD = dyn_cast<FunctionDecl>(D))
        FD->addAttr(AnnotateAttr::CreateImplicit(FD->getASTContext(),
                                                 "here it goes the annotation\n"));
    return true;
  }
private:
  // A RecursiveASTVisitor implementation.
  TaffoClassVisitor Visitor;
  
};

class TaffoClassVisitor
  : public RecursiveASTVisitor<TaffoClassVisitor> {
public:
  bool VisitVarDecl(VarDecl *Declaration) {
    // For debugging, dumping the AST nodes will show which nodes are already
    // being visited.
    Declaration->dump();

    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }
};

class TaffoClassAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<TaffoClassConsumer>();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  //to remove??
  PluginASTAction::ActionType getActionType() override {
    return AddBeforeMainAction;
  }
};



// preprocessing phase

class ExamplePragmaHandler : public PragmaHandler {
public:
  TaffoPragmaHandler() : PragmaHandler("taffo") { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    Token PragmaName = Tok;
    SmallVector<Token, 1> TokenList;
    PP.Lex(Tok);
    if (Tok.isNot(tok::identifier)) {
      printf("Error, a Taffo pragma must contain at least an option argument and a variable identifier\n");
     return;
    }
    auto *Info = new (PP.getPreprocessorAllocator()) PragmaTaffoInfo;
    if (!ParseTaffoValue(PP, Tok, PragmaName, *Info))
     return;


  if (Tok.isNot(tok::eod)) {
    printf("Error,  unexpected extra tokens at the end of pragma taffo\n");
    PP.Diag(Tok.getLocation(), diag::warn_pragma_extra_tokens_at_eol)
        << "taffo pragma";
    return;
  }

}

  static bool ParseTaffoValue(Preprocessor &PP, Token &Tok,Token PragmaName,  
                    PragmaTaffoInfo &Info) {
    SmallVector<std::string, 1> ValueList;
    while (Tok.isNot(tok::eod)) {
      IdentifierInfo *OptionInfo = Tok.getIdentifierInfo();
      ValueList.push_back(OptionInfo -> getName());
      PP.Lex(Tok);
    }
    
    Info.Toks = llvm::makeArrayRef(ValueList);
    Info.PragmaName = PragmaName;
    return true;
  } 
};






static FrontendPluginRegistry::Add<TaffoClassAction> X("taffo-plugin", "taffo plugin functions");
static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo","taffo pragma description");