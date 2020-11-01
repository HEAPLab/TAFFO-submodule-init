#ifndef TAFFO_INDIRECTCALLPATCHER_H
#define TAFFO_INDIRECTCALLPATCHER_H

#include "llvm/IR/Dominators.h"
#include <list>
#include <llvm/IR/CallSite.h>

namespace taffo {

    /** Function type for handlers **/
    using handler_function = void(*)(const llvm::Module &m, std::vector<llvm::Instruction *> &toDelete, llvm::CallInst *curCallInstruction,
                                     const llvm::CallSite *curCall, llvm::Function *indirectFunction);

    /** Latest allocated task name **/
    extern llvm::Function* allocatedTask;

    void manageIndirectCalls(llvm::Module &m);
}

#endif TAFFO_INDIRECTCALLPATCHER_H

