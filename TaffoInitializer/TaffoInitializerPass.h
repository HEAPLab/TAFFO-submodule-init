#include <limits>
#include <map>
#include "llvm/IR/CallSite.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "MultiValueMap.h"
#include "InputInfo.h"


#ifndef __TAFFO_INITIALIZER_PASS_H__
#define __TAFFO_INITIALIZER_PASS_H__


#define DEBUG_TYPE "taffo-init"
#define DEBUG_ANNOTATION "annotation"


STATISTIC(AnnotationCount, "Number of valid annotations found");
STATISTIC(FunctionCloned, "Number of fixed point function inserted");


namespace taffo {

struct ValueInfo {
  unsigned int backtrackingDepthLeft = 0;
  unsigned int fixpTypeRootDistance = UINT_MAX;

  std::shared_ptr<mdutils::MDInfo> metadata;
  llvm::Optional<std::string> target;
};


struct TaffoInitializer : public llvm::ModulePass {
  static char ID;
  
  using ConvQueueT = MultiValueMap<llvm::Value *, ValueInfo>;
  
  ConvQueueT globalQueue;
  std::map<llvm::Function *, ConvQueueT> functionQueues;
  
  ValueInfo *findValueInfo(llvm::Function *Func, llvm::Value *V) {
    auto F = globalQueue.find(V);
    if (F != globalQueue.end())
      return &(F->second);
    F = functionQueues[Func].find(V);
    if (F != functionQueues[Func].end())
      return &(F->second);
    return nullptr;
  }
  
  llvm::SmallPtrSet<llvm::Function *, 32> enabledFunctions;
  
  TaffoInitializer(): ModulePass(ID) { }
  bool runOnModule(llvm::Module &M) override;
  
  void readGlobalAnnotations(llvm::Module &m);
  void readLocalAnnotations(llvm::Function &f);
  void readAllLocalAnnotations(llvm::Module &m);
  llvm::Optional<ValueInfo> parseAnnotation(llvm::ConstantExpr *annoPtrInst, llvm::Value *instr, bool *isTarget = nullptr);
  void removeNoFloatTy(ConvQueueT& res);
  
  void buildConversionQueueForRootValues(llvm::Function *F, ConvQueueT& res);
  void createInfoOfUser(llvm::Value *used, const ValueInfo& VIUsed, llvm::Value *user, ValueInfo& VIUser);
  std::shared_ptr<mdutils::MDInfo> extractGEPIMetadata(const llvm::Value *user,
						       const llvm::Value *used,
						       std::shared_ptr<mdutils::MDInfo> user_mdi,
						       std::shared_ptr<mdutils::MDInfo> used_mdi);
  void generateFunctionSpace(ConvQueueT& vals);
  void generateFunctionSpace(ConvQueueT& vals, llvm::SmallPtrSetImpl<llvm::Function *> &callTrace);
  llvm::Function *createFunctionAndQueue(llvm::CallSite *call);
  void printConversionQueue(ConvQueueT& vals);
  void removeAnnotationCalls(ConvQueueT& vals);
  
  void setMetadataOfValue(llvm::Value *v, ValueInfo& VI);
  void setFunctionArgsMetadata(llvm::Module &m);

  bool isSpecialFunction(const llvm::Function* f) {
    llvm::StringRef fName = f->getName();
    return fName.startswith("llvm.") || f->getBasicBlockList().empty();
  };
};


}


#endif

