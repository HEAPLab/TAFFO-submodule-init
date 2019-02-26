#include <limits>
#include <llvm/IR/CallSite.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "InputInfo.h"


#ifndef __TAFFO_INITIALIZER_PASS_H__
#define __TAFFO_INITIALIZER_PASS_H__


#define DEBUG_TYPE "taffo-init"
#define DEBUG_ANNOTATION "annotation"


STATISTIC(AnnotationCount, "Number of valid annotations found");
STATISTIC(FunctionCloned, "Number of fixed point function inserted");


namespace taffo {

struct RangeError {
  double Min = std::numeric_limits<double>::quiet_NaN();
  double Max = std::numeric_limits<double>::quiet_NaN();
  double Error = std::numeric_limits<double>::quiet_NaN();
};

struct ValueInfo {
  bool isBacktrackingNode = false;
  bool isRoot;
  bool isOnlyRange = true;
  llvm::SmallPtrSet<llvm::Value*, 5> roots;
  int fixpTypeRootDistance = INT_MAX;
  
  std::shared_ptr<mdutils::MDInfo> metadata;
  llvm::Optional<std::string> target;
};


struct TaffoInitializer : public llvm::ModulePass {
  static char ID;

  /* to not be accessed directly, use valueInfo() */
  llvm::DenseMap<llvm::Value *, std::shared_ptr<ValueInfo>> info;
  
  TaffoInitializer(): ModulePass(ID) { }
  bool runOnModule(llvm::Module &M) override;
  
  void readGlobalAnnotations(llvm::Module &m, llvm::SmallPtrSetImpl<llvm::Value *>& res, bool functionAnnotation = false);
  void readLocalAnnotations(llvm::Function &f, llvm::SmallPtrSetImpl<llvm::Value *> &res);
  void readAllLocalAnnotations(llvm::Module &m, llvm::SmallPtrSetImpl<llvm::Value *> &res);
  bool parseAnnotation(llvm::SmallPtrSetImpl<llvm::Value *>& variables, llvm::ConstantExpr *annoPtrInst, llvm::Value *instr, bool *isTarget = nullptr);
  void removeNoFloatTy(llvm::SmallPtrSetImpl<llvm::Value *>& res);
  void printAnnotatedObj(llvm::Module &m);
  
  void buildConversionQueueForRootValues(const llvm::ArrayRef<llvm::Value*>& val, std::vector<llvm::Value*>& res);
  void generateFunctionSpace(std::vector<llvm::Value *> &vals, llvm::SmallPtrSetImpl<llvm::Value *> &global, llvm::SmallPtrSet<llvm::Function *, 10> &callTrace);
  llvm::Function *createFunctionAndQueue(llvm::CallSite *call, llvm::SmallPtrSetImpl<llvm::Value *> &global, std::vector<llvm::Value*> &convQueue);
  void printConversionQueue(std::vector<llvm::Value*> vals);
  void removeAnnotationCalls(std::vector<llvm::Value*>& vals);
  
  void setMetadataOfValue(llvm::Value *v);
  void setFunctionArgsMetadata(llvm::Module &m);

  bool isSpecialFunction(const llvm::Function* f) {
    llvm::StringRef fName = f->getName();
    return fName.startswith("llvm.") || f->getBasicBlockList().empty();
  };

  std::shared_ptr<ValueInfo> valueInfo(llvm::Value *val) {
    auto vi = info.find(val);
    if (vi == info.end()) {
      info[val] = std::make_shared<ValueInfo>(ValueInfo());
      return info[val];
    } else {
      return vi->getSecond();
    }
  };

  bool hasInfo(llvm::Value *val) {
    return info.find(val) != info.end();
};
};


}


#endif

