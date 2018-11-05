#include <sstream>
#include <iostream>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"
#include "TaffoInitializerPass.h"
#include "Metadata.h"

using namespace llvm;
using namespace taffo;


void TaffoInitializer::readGlobalAnnotations(Module &m, SmallPtrSetImpl<Value *>& variables,
					     bool functionAnnotation)
{
  GlobalVariable *globAnnos = m.getGlobalVariable("llvm.global.annotations");

  if (globAnnos != NULL)
  {
    if (ConstantArray *annos = dyn_cast<ConstantArray>(globAnnos->getInitializer()))
    {
      for (unsigned i = 0, n = annos->getNumOperands(); i < n; i++)
      {
        if (ConstantStruct *anno = dyn_cast<ConstantStruct>(annos->getOperand(i)))
        {
          /*struttura expr (operando 0 contiene expr) :
            [OpType] operandi :
            [BitCast] *funzione , [GetElementPtr] *annotazione ,
            [GetElementPtr] *filename , [Int] linea di codice sorgente) */
          if (ConstantExpr *expr = dyn_cast<ConstantExpr>(anno->getOperand(0)))
          {
            if (expr->getOpcode() == Instruction::BitCast && (functionAnnotation ^ !isa<Function>(expr->getOperand(0))) )
            {
              parseAnnotation(variables, cast<ConstantExpr>(anno->getOperand(1)), expr->getOperand(0));
            }
          }
        }
      }
    }
  }
  if (functionAnnotation)
    removeNoFloatTy(variables);
}


void TaffoInitializer::readLocalAnnotations(llvm::Function &f, llvm::SmallPtrSetImpl<llvm::Value *> &variables)
{
  bool found = false;
  for (inst_iterator iIt = inst_begin(&f), iItEnd = inst_end(&f); iIt != iItEnd; iIt++) {
    CallInst *call = dyn_cast<CallInst>(&(*iIt));
    if (call) {
      if (!call->getCalledFunction())
	continue;

      if (call->getCalledFunction()->getName() == "llvm.var.annotation") {
        parseAnnotation(variables, cast<ConstantExpr>(iIt->getOperand(1)), iIt->getOperand(0), &found);
      }
    }
  }
  if (found)
    mdutils::MetadataManager::setStartingPoint(f);
}


void TaffoInitializer::readAllLocalAnnotations(llvm::Module &m, llvm::SmallPtrSetImpl<llvm::Value *> &res)
{
  for (Function &f: m.functions()) {
    SmallPtrSet<Value*, 32> t;
    readLocalAnnotations(f, t);
    res.insert(t.begin(), t.end());

    /* Otherwise dce pass ignore the function
     * (removed also where it's not required) */
    f.removeFnAttr(Attribute::OptimizeNone);
  }
}

// Return true if the annotation contained only range data (no fixed point type data)
bool TaffoInitializer::parseAnnotation(SmallPtrSetImpl<Value *>& variables,
				       ConstantExpr *annoPtrInst, Value *instr,
				       bool *isTarget)
{
  ValueInfo vi;

  if (!(annoPtrInst->getOpcode() == Instruction::GetElementPtr))
    return false;
  GlobalVariable *annoContent = dyn_cast<GlobalVariable>(annoPtrInst->getOperand(0));
  if (!annoContent)
    return false;
  ConstantDataSequential *annoStr = dyn_cast<ConstantDataSequential>(annoContent->getInitializer());
  if (!annoStr)
    return false;
  if (!(annoStr->isString()))
    return false;

  StringRef annstr = annoStr->getAsString();
  std::istringstream strstm(annstr.substr(0, annstr.size()-1));

  bool readNumBits = true;
  std::string head;
  strstm >> head;
  if (head.find("target:") == 0) {
    vi.target = head.substr(7); // strlen("target:") == 7
    strstm >> head;
    if (isTarget)
      *isTarget = true;
  }
  if (head == "no_float")
    vi.isBacktrackingNode = false;
  else if (head == "force_no_float")
    vi.isBacktrackingNode = true;
  else if (head == "range")
    readNumBits = false;
  else
    return false;

  if (readNumBits) {
    int intbits, fracbits;
    strstm >> intbits >> fracbits;
    if (!strstm.fail()) {
      vi.fixpTypeRootDistance = 0;
      vi.fixpType.bitsAmt = intbits + fracbits;
      vi.fixpType.fracBitsAmt = fracbits;

      std::string signedflg;
      strstm >> signedflg;
      if (!strstm.fail() && signedflg == "unsigned") {
	vi.fixpType.isSigned = false;
      } else {
	vi.fixpType.isSigned = true;
      }
    }
  }

  // Look for Range info
  double Min, Max;
  strstm >> Min >> Max;
  if (!strstm.fail()) {
    vi.fixpTypeRootDistance = 0;
    vi.rangeError.Min = Min;
    vi.rangeError.Max = Max;
    LLVM_DEBUG(dbgs() << "Range found: [" << Min << ", " << Max << "]\n");

    // Look for initial error
    double Error;
    strstm >> Error;
    if (!strstm.fail()) {
      LLVM_DEBUG(dbgs() << "Initial error found " << Error << "\n");
      vi.rangeError.Error = Error;
    }
  }

  if (Instruction *toconv = dyn_cast<Instruction>(instr)) {
    variables.insert(toconv->getOperand(0));
    *valueInfo(toconv->getOperand(0)) = vi;
  } else {
    variables.insert(instr);
    *valueInfo(instr) = vi;
  }

  return !readNumBits;
}


void TaffoInitializer::removeNoFloatTy(SmallPtrSetImpl<Value *> &res)
{
  for (auto it: res) {
    Type *ty;

    AllocaInst *alloca;
    GlobalVariable *global;
    if ((alloca = dyn_cast<AllocaInst>(it))) {
      ty = alloca->getAllocatedType();
    } else if ((global = dyn_cast<GlobalVariable>(it))) {
      ty = global->getType();
    } else {
      LLVM_DEBUG(dbgs() << "annotated instruction " << *it <<
        " not an alloca or a global, ignored\n");
      res.erase(it);
      continue;
    }

    while (ty->isArrayTy() || ty->isPointerTy()) {
      if (ty->isPointerTy())
        ty = ty->getPointerElementType();
      else
        ty = ty->getArrayElementType();
    }
    if (!ty->isFloatingPointTy()) {
      LLVM_DEBUG(dbgs() << "annotated instruction " << *it << " does not allocate a"
        " kind of float; ignored\n");
      res.erase(it);
    }
  }
}

void TaffoInitializer::printAnnotatedObj(Module &m)
{
  SmallPtrSet<Value*, 32> res;

  readGlobalAnnotations(m, res, true);
  errs() << "Annotated Function: \n";
  if(!res.empty())
  {
    for (auto it : res)
    {
      errs() << " -> " << *it << "\n";
    }
    errs() << "\n";
  }

  res.clear();
  readGlobalAnnotations(m, res);
  errs() << "Global Set: \n";
  if(!res.empty())
  {
    for (auto it : res)
    {
      errs() << " -> " << *it << "\n";
    }
    errs() << "\n";
  }

  for (auto fIt=m.begin() , fItEnd=m.end() ; fIt!=fItEnd ; fIt++)
  {
    Function &f = *fIt;
    errs().write_escaped(f.getName()) << " : ";
    res.clear();
    readLocalAnnotations(f, res);
    if(!res.empty())
    {
      errs() << "\nLocal Set: \n";
      for (auto it : res)
      {
        errs() << " -> " << *it << "\n";
      }
    }
    errs() << "\n";
  }

}

