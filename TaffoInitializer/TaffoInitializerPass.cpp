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

  std::vector<llvm::Value *> rangeOnlyAnnotations;
  llvm::SmallPtrSet<llvm::Value *, 32> local;
  llvm::SmallPtrSet<llvm::Value *, 32> global;
  readAllLocalAnnotations(m, local, rangeOnlyAnnotations);
  readGlobalAnnotations(m, global);
  
  std::vector<Value*> rootsa(local.begin(), local.end());
  rootsa.insert(rootsa.begin(), global.begin(), global.end());
  AnnotationCount = rootsa.size();

  std::vector<Value*> vals;
  buildConversionQueueForRootValues(rootsa, vals);
  DEBUG(printConversionQueue(vals));

  for (Value *v: vals) {
    setMetadataOfValue(v);
  }
  setFunctionArgsMetadata(m);

  removeAnnotationCalls(rangeOnlyAnnotations);

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

  if (std::isnan(vi.rangeError.Min) || std::isnan(vi.rangeError.Max))
    return;

  mdutils::FPType fpty(vi.fixpType.bitsAmt, vi.fixpType.fracBitsAmt, vi.fixpType.isSigned);
  mdutils::Range range(vi.rangeError.Min, vi.rangeError.Max);
  mdutils::InputInfo II(&fpty, &range,
			  (std::isnan(vi.rangeError.Error) ? nullptr : &vi.rangeError.Error));

  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    if (vi.target.hasValue())
      mdutils::MetadataManager::setTargetMetadata(*inst, vi.target.getValue());

    if (!inst->getMetadata(INPUT_INFO_METADATA))
      mdutils::MetadataManager::setInputInfoMetadata(*inst, II);
  } else if (GlobalObject *con = dyn_cast<GlobalObject>(v)) {
    if (vi.target.hasValue())
      mdutils::MetadataManager::setTargetMetadata(*con, vi.target.getValue());

    if (!con->getMetadata(INPUT_INFO_METADATA))
      mdutils::MetadataManager::setInputInfoMetadata(*con, II);
  }
}

void TaffoInitializer::setFunctionArgsMetadata(Module &m)
{
  std::vector<mdutils::FPType> tyVec;
  std::vector<mdutils::Range> ranVec;
  std::vector<mdutils::InputInfo> iiVec;
  std::vector<mdutils::InputInfo *> iiPVec;
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
        const Value *sv = u.getUser();
        DEBUG(dbgs() << "Processing use " << *sv << "\n");
        if (isa<StoreInst>(sv)) {
          auto fVI = info.find(sv);
          if (fVI != info.end()) {
            DEBUG(dbgs() << "Info found.\n");
            ValueInfo &vi = *fVI->second;
            tyVec.push_back(mdutils::FPType(vi.fixpType.bitsAmt,
                                              vi.fixpType.fracBitsAmt,
                                              vi.fixpType.isSigned));
            ii.IType = &tyVec.back();

            ranVec.push_back(mdutils::Range(vi.rangeError.Min, vi.rangeError.Max));
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
        #ifdef LOG_BACKTRACK
        dbgs() << " - ";
        u->print(dbgs());
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
      auto oldrootsi = info.find(u);
      if (!info.count(u))
        continue;

      auto oldroots = oldrootsi->getSecond()->roots;
      SmallPtrSet<Value*, 5> merge(newroots);
      merge.insert(oldroots.begin(), oldroots.end());
      valueInfo(u)->roots = merge;
    }
  }
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

