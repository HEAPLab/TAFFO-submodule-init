#include <fstream>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"


#ifndef __FIXED_POINT_TYPE_H__
#define __FIXED_POINT_TYPE_H__


namespace taffo {


struct FixedPointType {
  bool isSigned;
  int fracBitsAmt;
  int bitsAmt;
  
  FixedPointType();
  FixedPointType(bool s, int f, int b) : isSigned(s), fracBitsAmt(f), bitsAmt(b) {};
  FixedPointType(llvm::Type *llvmtype, bool signd = true);
  llvm::Type *toLLVMType(llvm::LLVMContext& ctxt) const;
  std::string toString() const;

  bool operator==(const FixedPointType& rhs) const {
    return this->isSigned == rhs.isSigned &&
      this->fracBitsAmt == rhs.fracBitsAmt &&
      this->bitsAmt == rhs.bitsAmt;
  };
};


bool isFloatType(llvm::Type *srct);


}


llvm::raw_ostream& operator<<(llvm::raw_ostream& stm, const taffo::FixedPointType& f);


#endif

