#include "IndirectCallPatcher.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/IRBuilder.h>
#include "TaffoInitializerPass.h"
#include "TypeUtils.h"
#include "Metadata.h"

using namespace taffo;
using namespace llvm;

void handleKmpcFork(const Module &m, std::vector<Instruction *> &toDelete, CallInst *curCallInstruction,
                    const CallSite *curCall, Function *indirectFunction) {
  std::vector<Type *> paramsFunc;

  auto functionType = indirectFunction->getFunctionType();

  auto params = functionType->params();
  paramsFunc.reserve(2);
  for (auto i = 0; i < 2; i++) {
    paramsFunc.push_back(params[i]);
  }
  //The third argument (outlined function) may not be inserted as an argument as it's completely managed from the newF body
  for (auto i = 3; i < curCall->getNumArgOperands(); i++)
    paramsFunc.push_back(curCall->getArgOperand(i)->getType());

  auto newFunctionType = FunctionType::get(functionType->getReturnType(), paramsFunc, false);
  auto newFunctionName = indirectFunction->getName() + "_trampoline";
  Function *newF = Function::Create(
          newFunctionType, indirectFunction->getLinkage(),
          newFunctionName, indirectFunction->getParent());

  for (auto i = 3; i < curCall->getNumArgOperands(); i++) {
    // Shift back the argument name to set to take into account the skipped third argument
    newF->getArg(i - 1)->setName(curCall->getArgOperand(i)->getName());
  }

  BasicBlock *block = BasicBlock::Create(m.getContext(), "main", newF);

  auto microTaskOperand = dyn_cast<ConstantExpr>(curCall->arg_begin() + 2)->getOperand(0);
  auto microTaskFunction = dyn_cast<Function>(microTaskOperand);

  std::vector<Value *> convArgs;
  std::vector<Value *> trampolineArgs;

  // Create null pointer to patch the internal OpenMP argument
  Value *nullPointer = ConstantPointerNull::get(PointerType::get(Type::getInt32Ty(newF->getContext()), 0));
  convArgs.push_back(nullPointer);
  convArgs.push_back(nullPointer);

  for (auto argIt = curCall->arg_begin(); argIt < curCall->arg_begin() + 2; argIt += 1) {
    trampolineArgs.push_back(*argIt);
  }
  for (auto argIt = curCall->arg_begin() + 3; argIt < curCall->arg_end(); argIt += 1) {
    convArgs.push_back(*argIt);
    trampolineArgs.push_back(*argIt);
  }

  // Keep ref to the indirect function, preventing globaldce pass to destroy it
  auto magicBitCast = new BitCastInst(indirectFunction, indirectFunction->getType(), "", block);
  ReturnInst::Create(m.getContext(), nullptr, block);

  std::vector<Value *> outlinedArgumentsInsideTrampoline;

  outlinedArgumentsInsideTrampoline.push_back(nullPointer);
  outlinedArgumentsInsideTrampoline.push_back(nullPointer);

  for (auto argIt = newF->arg_begin() + 2; argIt < newF->arg_end(); argIt++) {
    outlinedArgumentsInsideTrampoline.push_back(argIt);
  }

  CallInst *outlinedCall = CallInst::Create(microTaskFunction, outlinedArgumentsInsideTrampoline);
  outlinedCall->insertAfter(magicBitCast);

  CallInst *trampolineCallInstruction = CallInst::Create(newF, trampolineArgs);
  trampolineCallInstruction->setCallingConv(curCallInstruction->getCallingConv());
  trampolineCallInstruction->insertBefore(curCallInstruction);
  trampolineCallInstruction->setDebugLoc(curCallInstruction->getDebugLoc());

  MDNode *indirectFunctionRef = MDNode::get(curCallInstruction->getContext(), ValueAsMetadata::get(indirectFunction));
  trampolineCallInstruction->setMetadata(INDIRECT_METADATA, indirectFunctionRef);

  toDelete.push_back(curCallInstruction);
  LLVM_DEBUG(dbgs() << "Newly created instruction: " << *trampolineCallInstruction << "\n");
}

const std::map<const std::string, handler_function> indirectCallFunctions = {
        {"__kmpc_fork_call", &handleKmpcFork},
//        {"__kmpc_omp_task_alloc", &handleCallToKmpcOmpTaskAlloc},
//        {"__kmpc_omp_task", &handleCallToKmpcOmpTask}
};

void *handleIndirectCall(const Module &m, std::vector<Instruction *> &toDelete, CallInst *curCallInstruction,
                         const CallSite *curCall, Function *indirectFunction) {
  indirectCallFunctions.find(indirectFunction->getName())->second(m, toDelete, curCallInstruction, curCall,
                                                                  indirectFunction);
}

bool isIndirectFunction(llvm::Function *function) {
  return indirectCallFunctions.count(function->getName());
}

void taffo::manageIndirectCalls(llvm::Module &m) {
  LLVM_DEBUG(dbgs() << "Checking Indirect Calls" << "\n");

  std::vector<Instruction *> toDelete;

  for (llvm::Function &curFunction : m) {
    for (auto instructionIt = inst_begin(curFunction); instructionIt != inst_end(curFunction); instructionIt++) {
      if (auto curCallInstruction = dyn_cast<CallInst>(&(*instructionIt))) {
        auto *curCall = new CallSite(curCallInstruction);
        llvm::Function *curCallFunction = curCall->getCalledFunction();

        if (curCallFunction && isIndirectFunction(curCallFunction)) {
          handleIndirectCall(m, toDelete, curCallInstruction, curCall, curCallFunction);
        }
      }
    }
  }

  // Delete the Instructions in a separate loop
  for (auto inst: toDelete) {
    inst->eraseFromParent();
  }
}