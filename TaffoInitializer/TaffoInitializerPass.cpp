#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "TaffoInitializerPass.h"


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
  printConversionQueue(vals);
  
  for (Value *v: vals) {
    setMetadataOfValue(v);
  }
  
  return true;
}


void TaffoInitializer::setMetadataOfValue(Value *v)
{
  ValueInfo& vi = info[v];
  LLVMContext& C = v->getContext();
  
  int realbm = vi.fixpType.isSigned ? -vi.fixpType.bitsAmt : vi.fixpType.bitsAmt;
  
  double min, max, epsilon;
  epsilon = pow(2, -vi.fixpType.fracBitsAmt);
  max = pow(2, vi.fixpType.bitsAmt) - epsilon;
  min = vi.fixpType.isSigned ? -max : 0.0;
  
  Metadata *MDs[] = {
    ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), realbm)),
    ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), vi.fixpType.fracBitsAmt)),
    ConstantAsMetadata::get(ConstantFP::get(Type::getDoubleTy(C), min)),
    ConstantAsMetadata::get(ConstantFP::get(Type::getDoubleTy(C), max))};
  MDNode *rangeMD = MDNode::get(C, MDs);

  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    inst->setMetadata("errorprop.range", rangeMD);
  } else if (GlobalObject *con = dyn_cast<GlobalObject>(v)) {
    Metadata *errorMDs[] = {
      ConstantAsMetadata::get(ConstantFP::get(Type::getDoubleTy(C), epsilon))};
    MDNode *errorMD = MDNode::get(C, errorMDs);
    Metadata *errorRangeMDs[] = {rangeMD, errorMD};
    MDNode *errorRangeMD = MDNode::get(C, errorRangeMDs);
    con->setMetadata("errorprop.globalre", errorRangeMD);
  }
}


void TaffoInitializer::buildConversionQueueForRootValues(
  const ArrayRef<Value*>& val,
  std::vector<Value*>& queue)
{
  queue.insert(queue.begin(), val.begin(), val.end());
  for (auto i = queue.begin(); i != queue.end(); i++) {
    info[*i].isRoot = true;
  }
  
  auto completeInfo = [this](Value *v, Value *u) {
    ValueInfo &vinfo = info[v];
    ValueInfo &uinfo = info[u];
    uinfo.origType = u->getType();
    if (uinfo.fixpTypeRootDistance > std::max(vinfo.fixpTypeRootDistance, vinfo.fixpTypeRootDistance+1)) {
      uinfo.fixpType = vinfo.fixpType;
      uinfo.fixpTypeRootDistance = std::max(vinfo.fixpTypeRootDistance, vinfo.fixpTypeRootDistance+1);
    }
  };

  size_t prevQueueSize = 0;
  while (prevQueueSize < queue.size()) {
    dbgs() << "***** buildConversionQueueForRootValues iter " << prevQueueSize << " < " << queue.size() << "\n";
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
        
        if (info[v].isBacktrackingNode) {
          info[u].isBacktrackingNode = true;
        }
        completeInfo(v, u);
      }
      next++;
    }
    
    next = queue.size();
    for (next = queue.size(); next != 0; next--) {
      Value *v = queue.at(next-1);
      if (!(info[v].isBacktrackingNode))
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
        info[v].isRoot = false;
        
        info[u].isBacktrackingNode = true;
        
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
          info[u].isRoot = true;
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
    if (info[v].isRoot) {
      info[v].roots = {v};
    }
    
    SmallPtrSet<Value*, 5> newroots = info[v].roots;
    for (Value *u: v->users()) {
      auto oldrootsi = info.find(u);
      if (oldrootsi == info.end())
        continue;
      
      auto oldroots = oldrootsi->getSecond().roots;
      SmallPtrSet<Value*, 5> merge(newroots);
      merge.insert(oldroots.begin(), oldroots.end());
      info[u].roots = merge;
    }
  }
}


void TaffoInitializer::printConversionQueue(std::vector<Value*> vals)
{
  if (vals.size() < 1000) {
    errs() << "conversion queue:\n";
                  for (Value *val: vals) {
                    errs() << "bt=" << info[val].isBacktrackingNode << " ";
                    errs() << info[val].fixpType << " ";
                    errs() << "[";
                    for (Value *rootv: info[val].roots) {
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

