#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"


#ifndef __TAFFO_INITIALIZER_PASS_H__
#define __TAFFO_INITIALIZER_PASS_H__


#define DEBUG_TYPE "flttofix"
#define DEBUG_ANNOTATION "annotation"


STATISTIC(AnnotationCount, "Number of valid annotations found");


namespace taffo {


struct TaffoInitializer : public llvm::ModulePass {
  static char ID;
  
  TaffoInitializer(): ModulePass(ID) { }
  bool runOnModule(llvm::Module &M) override;
};


}


#endif

