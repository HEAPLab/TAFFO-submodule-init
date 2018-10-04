#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"
#include "FixedPointType.h"
#include <sstream>


using namespace llvm;
using namespace taffo;


cl::opt<int> GlobalFracBitsAmt("fixpfracbitsamt", cl::value_desc("bits"),
  cl::desc("Default amount of fractional bits in fixed point numbers"),
  cl::init(16));
cl::opt<int> GlobalBitsAmt("fixpbitsamt", cl::value_desc("bits"),
  cl::desc("Default amount of bits in fixed point numbers"), cl::init(32));


FixedPointType::FixedPointType()
{
  this->isSigned = true;
  this->fracBitsAmt = GlobalFracBitsAmt;
  this->bitsAmt = GlobalBitsAmt;
}


FixedPointType::FixedPointType(Type *llvmtype, bool signd)
{
  this->isSigned = signd;
  if (isFloatType(llvmtype)) {
    this->fracBitsAmt = GlobalFracBitsAmt;
    this->bitsAmt = GlobalBitsAmt;
    return;
  } else if (llvmtype->isIntegerTy()) {
    this->fracBitsAmt = 0;
    this->bitsAmt = llvmtype->getIntegerBitWidth();
  } else {
    this->fracBitsAmt = 0;
    this->bitsAmt = 0;
  }
}


Type *FixedPointType::toLLVMType(LLVMContext& ctxt) const
{
  return Type::getIntNTy(ctxt, this->bitsAmt);
}


std::string FixedPointType::toString() const
{
  std::stringstream stm;
  if (isSigned)
    stm << "s";
  else
    stm << "u";
  stm << bitsAmt - fracBitsAmt << "_" << fracBitsAmt << "fixp";
  return stm.str();
}


raw_ostream& operator<<(raw_ostream& stm, const FixedPointType& f)
{
  stm << f.toString();
  return stm;
}


bool taffo::isFloatType(Type *srct)
{
  if (srct->isPointerTy()) {
    return isFloatType(srct->getPointerElementType());
  } else if (srct->isArrayTy()) {
    return isFloatType(srct->getArrayElementType());
  } else if (srct->isFloatingPointTy()) {
    return true;
  }
  return false;
}


