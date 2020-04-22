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
#include "AnnotationParser.h"
#include "Metadata.h"

using namespace llvm;
using namespace taffo;


void TaffoInitializer::readGlobalAnnotations(Module &m)
{
  GlobalVariable *globAnnos = m.getGlobalVariable("llvm.global.annotations");
  if (!globAnnos)
    return;
    
  ConstantArray *annos = dyn_cast<ConstantArray>(globAnnos->getInitializer());
  if (!annos)
    return;
    
  for (auto &Opnd: annos->operands()) {
    ConstantStruct *anno = dyn_cast<ConstantStruct>(Opnd.get());
    if (!anno)
      continue;
      
    /*struttura expr (operando 0 contiene expr) :
      [OpType] operandi :
      [BitCast] *funzione , [GetElementPtr] *annotazione ,
      [GetElementPtr] *filename , [Int] linea di codice sorgente) */
    ConstantExpr *expr = dyn_cast<ConstantExpr>(anno->getOperand(0));
    if (!expr)
      continue;
    if (expr->getOpcode() != Instruction::BitCast)
      continue;
    
    ConstantExpr *AnnStr = cast<ConstantExpr>(anno->getOperand(1));
    Value *AnnVal = expr->getOperand(0);
    auto Ann = parseAnnotation(AnnStr, AnnVal);
    if (!Ann)
      continue;
      
    if (Function *F = dyn_cast_or_null<Function>(AnnVal)) {
      enabledFunctions.insert(F);
      for (auto user: F->users()) {
        CallBase *Call = dyn_cast<CallBase>(user);
        if (!Call)
          continue;
        Function *PF = Call->getFunction();
        ConvQueueT &FQ = functionQueues[PF];
        FQ.push_back(user, Ann.getValue());
      }
      
    } else {
      globalQueue.push_back(AnnVal, Ann.getValue());
    }
  }
}


void TaffoInitializer::readLocalAnnotations(llvm::Function &f)
{
  ConvQueueT &Q = functionQueues[&f];
  
  bool found = false;
  for (inst_iterator iIt = inst_begin(&f), iItEnd = inst_end(&f); iIt != iItEnd; iIt++) {
    if (CallInst *call = dyn_cast<CallInst>(&(*iIt))) {
      if (!call->getCalledFunction())
        continue;
      if (call->getCalledFunction()->getName() != "llvm.var.annotation")
        continue;
        
      bool startingPoint = false;
      Value *V = iIt->getOperand(0);
      auto Ann = parseAnnotation(cast<ConstantExpr>(iIt->getOperand(1)), V, &startingPoint);
      if (!Ann)
        continue;
      
      if (Instruction *toconv = dyn_cast<Instruction>(V)) {
        Q.push_back(toconv->getOperand(0), Ann.getValue());
      } else {
        Q.push_back(V, Ann.getValue());
      }
      
      found |= startingPoint;
    }
  }
  if (found) {
    mdutils::MetadataManager::setStartingPoint(f);
  }
}


void TaffoInitializer::readAllLocalAnnotations(llvm::Module &m)
{
  for (Function &f: m.functions()) {
    readLocalAnnotations(f);

    /* Otherwise the dce pass -- and other passes as well -- will ignore the
     * function (removed also where it's not required) */
    f.removeFnAttr(Attribute::OptimizeNone);
  }
}

// Return true on success, false on error
Optional<ValueInfo> TaffoInitializer::parseAnnotation(
    ConstantExpr *annoPtrInst, Value *instr, bool *startingPoint)
{
  ValueInfo vi;

  if (!(annoPtrInst->getOpcode() == Instruction::GetElementPtr))
    return None;
  GlobalVariable *annoContent = dyn_cast<GlobalVariable>(annoPtrInst->getOperand(0));
  if (!annoContent)
    return None;
  ConstantDataSequential *annoStr = dyn_cast<ConstantDataSequential>(annoContent->getInitializer());
  if (!annoStr)
    return None;
  if (!(annoStr->isString()))
    return None;

  StringRef annstr = annoStr->getAsString();
  AnnotationParser parser;
  if (!parser.parseAnnotationString(annstr)) {
    errs() << "TAFFO annnotation parser syntax error: \n";
    errs() << "  In annotation: \"" << annstr << "\"\n";
    errs() << "  " << parser.lastError() << "\n";
    return None;
  }
  
  AnnotationCount++;
  
  vi.fixpTypeRootDistance = 0;
  if (!parser.backtracking)
    vi.backtrackingDepthLeft = 0;
  else
    vi.backtrackingDepthLeft = parser.backtrackingDepth;
  vi.metadata = parser.metadata;
  if (startingPoint)
    *startingPoint = parser.startingPoint;
  vi.target = parser.target;
  return vi;
}


void TaffoInitializer::removeNoFloatTy(MultiValueMap<Value *, ValueInfo>& res)
{
  for (auto PIt: res) {
    Type *ty;
    Value *it = PIt->first;

    if (AllocaInst *alloca = dyn_cast<AllocaInst>(it)) {
      ty = alloca->getAllocatedType();
    } else if (GlobalVariable *global = dyn_cast<GlobalVariable>(it)) {
      ty = global->getType();
    } else if (isa<CallInst>(it) || isa<InvokeInst>(it)) {
      ty = it->getType();
      if (ty->isVoidTy())
        continue;
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

