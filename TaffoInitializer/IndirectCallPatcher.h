#ifndef TAFFO_INDIRECTCALLPATCHER_H
#define TAFFO_INDIRECTCALLPATCHER_H

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Dominators.h"
#include <list>

namespace taffo {

/** Function type for handlers **/
using handler_function = void (*)(const llvm::Module &m,
                                  std::vector<llvm::Instruction *> &toDelete,
                                  llvm::CallInst *curCallInstruction,
                                  const llvm::CallSite *curCall,
                                  llvm::Function *indirectFunction);

void manageIndirectCalls(llvm::Module &m);
} // namespace taffo

#endif TAFFO_INDIRECTCALLPATCHER_H
