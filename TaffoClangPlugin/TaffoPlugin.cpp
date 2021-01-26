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
    bool used;
  };

// 1048 annotations is the maximum allowed for a compilation unit
static SmallVector<PragmaTaffoInfo,1048> InfoList;


class TaffoPragmaVisitor
  : public RecursiveASTVisitor<TaffoPragmaVisitor> {
public:
  explicit TaffoPragmaVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitVarDecl(VarDecl *Declaration) {
    // For debugging, dumping the AST nodes will show which nodes are being visited
    //Declaration->dump();

    //getting the variable name
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
      if(!info.used && info.ID.compare(Vname)==0 && info.funName.compare(Fname)==0){
        Declaration->addAttr(AnnotateAttr::CreateImplicit(Declaration->getASTContext(),
                                                 info.annotation));
        //an annotation can be used just once
        info.used=true;
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
    //looking in the list if this function declaration has been annotated by the user
    for(PragmaTaffoInfo info : InfoList){
      if(!info.used && info.ID.compare(Fname)==0 && info.funName.compare("")==0){
        Declaration->addAttr(AnnotateAttr::CreateImplicit(Declaration->getASTContext(),
                                                 info.annotation));

        //an annotation can be used just once
        info.used = true;
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
      PP.Diag(Tok.getLocation(), diag::warn_pragma_expected_identifier) << "taffo";
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

    //parsing the actual annotation
    if (Tok.is(tok::eod)) {
      PP.Diag(Tok.getLocation(), diag::warn_pragma_missing_argument) << "taffo";
      return;
    }
    std::string ann = Tok.getLiteralData();
    ann = ann.substr(0, ann.find("\n"));  
    //in indicates whether we are reading a char inside two matching double quotes, or outside
    bool in = false;
    //the parsed annotation
    std::string parsed = "";
    int pos =0;
    while (pos < ann.size()){
      if(ann[pos] == '\"'){
        in = !in;
      }
      else{
        if(!in && ann[pos] != ' ' ){
          //if we read some chars outside double quotes, the annotation is not valid
          PP.Diag(Tok.getLocation(), diag::warn_pragma_invalid_argument) << "taffo";
          return;
        }
        if(in){
          //characters inside double quotes form the annotation
          parsed.push_back(ann[pos]);
        }
      }
      pos++;
    }
    //if a quote is not closed, the annotation is not valid
    if (in){
      PP.Diag(Tok.getLocation(), diag::warn_pragma_invalid_argument) << "taffo";
      return;
    }
    PP.Lex(Tok);


    // extra tokens at the end of taffo pragma are added in case macros are used, and must be parsed and thrown away as well
    std::string extra = "";
    if (Tok.isNot(tok::eod)) {
      while (Tok.isNot(tok::eod)){
        PP.Lex(Tok);
      }
    }

    Info.annotation = parsed;
    Info.used = false;

    //checking whether this variable has already been annotated
    // we always just keep the first annotation
    for(PragmaTaffoInfo info : InfoList){
      if(info.ID.compare(Info.ID)==0 && info.funName.compare(Info.funName)==0){
        PP.Diag(Tok.getLocation(), diag::warn_pragma_invalid_argument) << "taffo";
        return;
      }
    }

    InfoList.push_back(Info);

  } 
};

}

static FrontendPluginRegistry::Add<TaffoPragmaAction> X("taffo-plugin", "taffo plugin functions");
static PragmaHandlerRegistry::Add<TaffoPragmaHandler> Y("taffo","taffo pragma description");