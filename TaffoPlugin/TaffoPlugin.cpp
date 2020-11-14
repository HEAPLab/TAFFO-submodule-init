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

static bool EnableAnnotate = false;
static bool HandledDecl = false;
static SmallVector<std::string,32>annotations;
static std::string VarName;

class TaffoFunctionsConsumer : public ASTConsumer {
public:
  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    HandledDecl = true;
    if (!EnableAnnotate)
      return true;
    for (auto D : DG)
      if (FunctionDecl *FD = dyn_cast<FunctionDecl>(D))
        FD->addAttr(AnnotateAttr::CreateImplicit(FD->getASTContext(),
                                                 VarName));
    return true;
  }
};

class TaffoFunctionsAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return std::make_unique<TaffoFunctionsConsumer>();
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
  Token Option = Tok;
  IdentifierInfo *OptionInfo = Tok.getIdentifierInfo();
    bool OptionValid = llvm::StringSwitch<bool>(OptionInfo->getName())
                           .Case("target", true)
                           .Case("backtracking", true)
                           .Case("errtarget", true)
                           .Case("scalar", true)
                           .Case("error", true)
                           .Case("disabled",true)
                           .Case("final",true)
                           .Default(false);

  if (!OptionValid) {
    printf("Error, option not recognized for pragma taffo\n");
    return;
  }
  PP.Lex(Tok);

  auto *Info = new (PP.getPreprocessorAllocator()) PragmaTaffoInfo;
  if (!ParseTaffoValue(PP, Tok, PragmaName, Option, *Info))
    return;


  Token TaffoTok;
  TaffoTok.startToken();
  TaffoTok.setKind(tok::annot_pragma_taffo);
  TaffoTok.setLocation(Info->PragmaName.getLocation());
  TaffoTok.setAnnotationEndLoc(Info->PragmaName.getLocation());
  TaffoTok.setAnnotationValue(static_cast<void *>(Info));
  TokenList.push_back(TaffoTok);
  if (Tok.isNot(tok::eod)) {
    printf("Error, extra tokens at the end of pragma taffo\n");
    PP.Diag(Tok.getLocation(), diag::warn_pragma_extra_tokens_at_eol)
        << "quality pragma";
    return;
  }


  
  auto TokenArray = std::make_unique<Token[]>(TokenList.size());
  std::copy(TokenList.begin(), TokenList.end(), TokenArray.get());
  PP.EnterTokenStream(std::move(TokenArray), TokenList.size(),
                      /*DisableMacroExpansion=*/false, /*IsReinject=*/false);
  }

  static bool ParseTaffoValue(Preprocessor &PP, Token &Tok,Token PragmaName,  Token Option, 
                    PragmaTaffoInfo &Info) {
    SmallVector<Token, 1> ValueList;
    while (Tok.isNot(tok::eod)) {
      ValueList.push_back(Tok);
      PP.Lex(Tok);
    }
    Token EOFTok;
    EOFTok.startToken();
    EOFTok.setKind(tok::eof);
    EOFTok.setLocation(Tok.getLocation());
    ValueList.push_back(EOFTok); // Terminates expression for parsing.
    Info.Toks = llvm::makeArrayRef(ValueList).copy(PP.getPreprocessorAllocator());
    Info.Option = Option;
    Info.PragmaName = PragmaName;
    return true;
  } 
};






static FrontendPluginRegistry::Add<TaffoFunctionsAction> X("annotate-fns", "annotate functions");


static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo_pragma","taffo pragma descption description");