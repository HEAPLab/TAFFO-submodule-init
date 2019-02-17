#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "TaffoInitializerPass.h"
#include <cmath>

#include "Metadata.h"


using namespace llvm;
using namespace taffo;


char TaffoInitializer::ID = 0;

static RegisterPass<TaffoInitializer> X(
  "taffoinit",
  "TAFFO Framework Initialization Stage",
  false /* does not affect the CFG */,
  true /* Optimization Pass (sorta) */);

bool TaffoInitializer::runOnModule(Module &m)
{
  DEBUG_WITH_TYPE(DEBUG_ANNOTATION, printAnnotatedObj(m));

  llvm::SmallPtrSet<llvm::Value *, 32> local;
  llvm::SmallPtrSet<llvm::Value *, 32> global;
  readAllLocalAnnotations(m, local);
  readGlobalAnnotations(m, global);

  std::vector<Value*> rootsa(local.begin(), local.end());
  rootsa.insert(rootsa.begin(), global.begin(), global.end());
  AnnotationCount = rootsa.size();

  std::vector<Value*> vals;
  buildConversionQueueForRootValues(rootsa, vals);
  for (Value *v: vals) {
    setMetadataOfValue(v);
  }
  removeAnnotationCalls(vals);

  SmallPtrSet<Function*, 10> callTrace;
  generateFunctionSpace(vals, global, callTrace);

  DEBUG(printConversionQueue(vals));
  setFunctionArgsMetadata(m);

  return true;
}


void TaffoInitializer::removeAnnotationCalls(std::vector<Value*>& q)
{
  for (auto i = q.begin(); i != q.end();) {
    Value *v = *i;
    
    if (CallInst *anno = dyn_cast<CallInst>(v)) {
      if (anno->getCalledFunction()) {
        if (anno->getCalledFunction()->getName() == "llvm.var.annotation") {
          anno->eraseFromParent();
          i = q.erase(i);
          continue;
        }
      }
    }
    
    i++;
  }
}


void TaffoInitializer::setMetadataOfValue(Value *v)
{
  ValueInfo& vi = *valueInfo(v);

  mdutils::FPType fpty(0, 0, false);
  mdutils::Range range(vi.rangeError.Min, vi.rangeError.Max);
  mdutils::InputInfo II;

  //set MetaData only for annotated instruction
  II = mdutils::InputInfo(
      vi.isOnlyRange ? nullptr : &fpty,
      vi.fixpTypeRootDistance ? new mdutils::Range(RangeError().Min,RangeError().Max) : &range,
      std::isnan(vi.rangeError.Error) ? nullptr : &vi.rangeError.Error);

  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    if (vi.target.hasValue())
      mdutils::MetadataManager::setTargetMetadata(*inst, vi.target.getValue());

    mdutils::MetadataManager::setInputInfoMetadata(*inst, II);
  } else if (GlobalObject *con = dyn_cast<GlobalObject>(v)) {
    if (vi.target.hasValue())
      mdutils::MetadataManager::setTargetMetadata(*con, vi.target.getValue());

    mdutils::MetadataManager::setInputInfoMetadata(*con, II);
  }
}

void TaffoInitializer::setFunctionArgsMetadata(Module &m)
{
  std::vector<mdutils::FPType> tyVec;
  std::vector<mdutils::Range> ranVec;
  std::vector<mdutils::InputInfo> iiVec;
  std::vector<mdutils::MDInfo *> iiPVec;
  for (Function &f : m.functions()) {
    DEBUG(dbgs() << "Processing function " << f.getName() << "\n");
    tyVec.reserve(f.arg_size());
    ranVec.reserve(f.arg_size());
    iiVec.reserve(f.arg_size());
    iiPVec.reserve(f.arg_size());

    for (const Argument &a : f.args()) {
      DEBUG(dbgs() << "Processing arg " << a << "\n");
      mdutils::InputInfo ii(nullptr, nullptr, nullptr);
      for (const Use &u : a.uses()) {
        Value *sv = u.getUser();
        DEBUG(dbgs() << "Processing use " << *sv << "\n");
        if (isa<StoreInst>(sv)) {
          if (hasInfo(sv)) {
            DEBUG(dbgs() << "Info found.\n");
            ValueInfo &vi = *valueInfo(sv);
            tyVec.push_back(mdutils::FPType(0,0, false));
            ii.IType = vi.isOnlyRange ? nullptr : &tyVec.back();

            mdutils::Range rng = vi.fixpTypeRootDistance
                ? mdutils::Range(vi.rangeError.Min, vi.rangeError.Max)
                : mdutils::Range(RangeError().Min,RangeError().Max);
            ranVec.push_back(rng);
            ii.IRange = &ranVec.back();

            ii.IError = std::isnan(vi.rangeError.Error) ? nullptr : &vi.rangeError.Error;

            break;
          }
        }
      }
      iiVec.push_back(ii);
      iiPVec.push_back(&iiVec.back());
    }

    mdutils::MetadataManager::setArgumentInputInfoMetadata(f, iiPVec);

    tyVec.clear();
    ranVec.clear();
    iiVec.clear();
    iiPVec.clear();
  }
}


void TaffoInitializer::buildConversionQueueForRootValues(
  const ArrayRef<Value*>& val,
  std::vector<Value*>& queue)
{
  queue.insert(queue.begin(), val.begin(), val.end());
  for (auto i = queue.begin(); i != queue.end(); i++) {
    valueInfo(*i)->isRoot = true;
  }

  auto completeInfo = [this](Value *v, Value *u) {
    ValueInfo vinfo = *valueInfo(v);
    ValueInfo &uinfo = *valueInfo(u);
    uinfo.origType = u->getType();
    if (uinfo.fixpTypeRootDistance > std::max(vinfo.fixpTypeRootDistance, vinfo.fixpTypeRootDistance+1)) {
      uinfo.fixpType = vinfo.fixpType;
      uinfo.rangeError.Min = vinfo.rangeError.Min;
      uinfo.rangeError.Max = vinfo.rangeError.Max;
      uinfo.rangeError.Error = vinfo.rangeError.Error;
      uinfo.target = vinfo.target;
      uinfo.fixpTypeRootDistance = std::max(vinfo.fixpTypeRootDistance, vinfo.fixpTypeRootDistance+1);
    }
    uinfo.isOnlyRange = uinfo.isOnlyRange && vinfo.isOnlyRange;
  };

  size_t prevQueueSize = 0;
  while (prevQueueSize < queue.size()) {
    DEBUG(dbgs() << "***** buildConversionQueueForRootValues iter " << prevQueueSize << " < " << queue.size() << "\n";);
    prevQueueSize = queue.size();

    size_t next = 0;
    while (next < queue.size()) {
      Value *v = queue.at(next);

      for (auto *u: v->users()) {

        /* Insert u at the end of the queue.
         * If u exists already in the queue, *move* it to the end instead. */
        for (int i=0; i<queue.size();) {
          if (queue[i] == u) {
            queue.erase(queue.begin() + i);
            if (i < next)
              next--;
          } else {
            i++;
          }
        }
        queue.push_back(u);

        if (valueInfo(v)->isBacktrackingNode) {
          valueInfo(u)->isBacktrackingNode = true;
        }
        completeInfo(v, u);
      }
      next++;
    }

    next = queue.size();
    for (next = queue.size(); next != 0; next--) {
      Value *v = queue.at(next-1);
      if (!(valueInfo(v)->isBacktrackingNode))
        continue;

      Instruction *inst = dyn_cast<Instruction>(v);
      if (!inst)
        continue;

      #ifdef LOG_BACKTRACK
      dbgs() << "BACKTRACK ";
      v->print(dbgs());
      dbgs() << "\n";
      #endif

      for (Value *u: inst->operands()) {
        if (!isa<User>(u) && !isa<Argument>(u)) {
          #ifdef LOG_BACKTRACK
          dbgs() << " - " ;
          u->printAsOperand(dbgs());
          dbgs() << " not a User or an Argument\n";
          #endif
          continue;
        }

        if (isa<Function>(u) || isa<BlockAddress>(u)) {
          #ifdef LOG_BACKTRACK
          dbgs() << " - " ;
          u->printAsOperand(dbgs());
          dbgs() << " is a function/block address\n";
          #endif
          continue;
        }

        #ifdef LOG_BACKTRACK
        dbgs() << " - " << *u;
        #endif

        if (!isFloatType(u->getType())) {
          #ifdef LOG_BACKTRACK
          dbgs() << " not a float\n";
          #endif
          continue;
        }
        valueInfo(v)->isRoot = false;

        valueInfo(u)->isBacktrackingNode = true;

        bool alreadyIn = false;
        for (int i=0; i<queue.size() && !alreadyIn;) {
          if (queue[i] == u) {
            if (i < next)
              alreadyIn = true;
            else
              queue.erase(queue.begin() + i);
          } else {
            i++;
          }
        }
        if (!alreadyIn) {
          valueInfo(u)->isRoot = true;
          #ifdef LOG_BACKTRACK
          dbgs() << "  enqueued\n";
          #endif
          queue.insert(queue.begin()+next-1, u);
          next++;
        } else {
          #ifdef LOG_BACKTRACK
          dbgs() << " already in\n";
          #endif
        }

        completeInfo(v, u);
      }
    }
  }

  for (Value *v: queue) {
    if (valueInfo(v)->isRoot) {
      valueInfo(v)->roots = {v};
    }

    SmallPtrSet<Value*, 5> newroots = valueInfo(v)->roots;
    for (Value *u: v->users()) {
      if (!hasInfo(u))
        continue;

      auto oldroots = valueInfo(u)->roots;
      SmallPtrSet<Value*, 5> merge(newroots);
      merge.insert(oldroots.begin(), oldroots.end());
      valueInfo(u)->roots = merge;
    }
  }
}


void TaffoInitializer::generateFunctionSpace(std::vector<Value *> &vals, SmallPtrSetImpl<llvm::Value *> &global,
                                             SmallPtrSet<Function *, 10> &callTrace)
{
  std::vector<CallSite*> calls;

  //filter
  for (Value *v : vals) {
    if (isa<CallInst>(v) || isa<InvokeInst>(v)) {
      CallSite *call = new CallSite(v);
      calls.push_back(call);
    }
  }

  for (CallSite *call : calls) {
    Function *oldF = call->getCalledFunction();
    assert(oldF && "bitcasted function pointers and such not handled atm");
    if(isSpecialFunction(oldF))
      continue;

    std::vector<Value*> newVals;
    Function *newF = createFunctionAndQueue(call, global, newVals);
    call->setCalledFunction(newF);

    //Attach metadata
    MDNode *newFRef = MDNode::get(call->getInstruction()->getContext(),ValueAsMetadata::get(newF));
    MDNode *oldFRef = MDNode::get(call->getInstruction()->getContext(),ValueAsMetadata::get(oldF));

    call->getInstruction()->setMetadata(ORIGINAL_FUN_METADATA, oldFRef);
    if (MDNode *cloned = oldF->getMetadata(CLONED_FUN_METADATA)) {
      cloned = cloned->concatenate(cloned, newFRef);
      oldF->setMetadata(CLONED_FUN_METADATA, cloned);
    } else {
      oldF->setMetadata(CLONED_FUN_METADATA, newFRef);
    }
    newF->setMetadata(CLONED_FUN_METADATA, NULL);
    newF->setMetadata(SOURCE_FUN_METADATA, oldFRef);
    for (Value *v : newVals) {
      setMetadataOfValue(v);
    }

    if (callTrace.count(oldF)) {
      continue;
    }
    callTrace.insert(oldF);
    callTrace.insert(newF);
    generateFunctionSpace(newVals, global, callTrace);
    callTrace.erase(newF);
    callTrace.erase(oldF);
  }
}


Function* TaffoInitializer::createFunctionAndQueue(CallSite *call, SmallPtrSetImpl<Value *> &global,
    std::vector<Value *> &convQueue)
{
  Function *oldF = call->getCalledFunction();
  Function *newF = Function::Create(
      oldF->getFunctionType(), oldF->getLinkage(),
      oldF->getName(), oldF->getParent());

  ValueToValueMapTy mapArgs; // Create Val2Val mapping and clone function
  Function::arg_iterator newIt = newF->arg_begin();
  Function::arg_iterator oldIt = oldF->arg_begin();
  for (; oldIt != oldF->arg_end() ; oldIt++, newIt++) {
    newIt->setName(oldIt->getName());
    mapArgs.insert(std::make_pair(oldIt, newIt));
  }
  SmallVector<ReturnInst*,100> returns;
  CloneFunctionInto(newF, oldF, mapArgs, true, returns);
  FunctionCloned++;

  std::vector<Value*> roots; //propagate fixp conversion
  oldIt = oldF->arg_begin();
  newIt = newF->arg_begin();
  DEBUG(dbgs() << "Create function from " << oldF->getName() << " to " << newF->getName() << "\n";);
  for (int i=0; oldIt != oldF->arg_end() ; oldIt++, newIt++,i++) {
    if (hasInfo(call->getInstruction()->getOperand(i))) {
      RangeError rng = valueInfo(call->getInstruction()->getOperand(i))->rangeError;
      int dist = valueInfo(call->getInstruction()->getOperand(i))->fixpTypeRootDistance;
      bool isOnlyRange = valueInfo(call->getInstruction()->getOperand(i))->isOnlyRange;

      // Mark the alloca used for the argument (in O0 opt lvl)
      valueInfo(newIt->user_begin()->getOperand(1))->rangeError = rng;
      valueInfo(newIt->user_begin()->getOperand(1))->fixpTypeRootDistance = dist+2;
      valueInfo(newIt->user_begin()->getOperand(1))->isOnlyRange = isOnlyRange;
      roots.push_back(newIt->user_begin()->getOperand(1));

      DEBUG(dbgs() << "\tArg nr. " << i << " has range [" << rng.Min << " , " << rng.Max << "]\n";);
      DEBUG(dbgs() << *newIt->user_begin()->getOperand(1) <<" "
                   << valueInfo(newIt->user_begin()->getOperand(1))->rangeError.Min << " - "
                   << valueInfo(newIt->user_begin()->getOperand(1))->rangeError.Max << "\n";);

      // Mark the argument itself
      valueInfo(newIt)->rangeError = rng;
      valueInfo(newIt)->fixpTypeRootDistance = dist+1;
      valueInfo(newIt)->isOnlyRange = isOnlyRange;
    }
  }

  std::vector<Value*> tmpVals;
  roots.insert(roots.begin(), global.begin(), global.end());
  SmallPtrSet<Value*, 32> localFix;
  readLocalAnnotations(*newF, localFix);
  roots.insert(roots.begin(), localFix.begin(), localFix.end());
  buildConversionQueueForRootValues(roots, tmpVals);
  for (Value *val : tmpVals){
    if (Instruction *inst = dyn_cast<Instruction>(val)) {
      if (inst->getFunction()==newF){
        convQueue.push_back(val);
      }
    }
  }
  return newF;
}


void TaffoInitializer::printConversionQueue(std::vector<Value*> vals)
{
  if (vals.size() < 1000) {
    errs() << "conversion queue:\n";
    for (Value *val: vals) {
      errs() << "bt=" << valueInfo(val)->isBacktrackingNode << " ";
      errs() << valueInfo(val)->fixpType << " ";
      errs() << "[";
      for (Value *rootv: valueInfo(val)->roots) {
        rootv->print(errs());
        errs() << ' ';
      }
      errs() << "] ";
      val->print(errs());
      errs() << "\n";
    }
    errs() << "\n\n";
  } else {
    errs() << "not printing the conversion queue because it exceeds 1000 items";
  }
}

