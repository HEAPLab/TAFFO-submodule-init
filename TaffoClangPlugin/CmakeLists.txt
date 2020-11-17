cmake_policy (SET CMP0057 NEW)

# Find LLVM (this looks for LLVMConfig.cmake somewhere in the PATH or LLVM_DIR).
find_package(LLVM REQUIRED CONFIG)

# Enable us to use the LLVM CMake modules.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")

# Enable us to use add_llvm_library.
include(AddLLVM)

# Bring in the LLVM include directories.
include_directories(${LLVM_INCLUDE_DIRS})

# Bring in any definitions LLVM requires.
add_definitions(${LLVM_DEFINITIONS})


# If we don't need RTTI or EH, there's no reason to export anything
# from the plugin.
if( NOT MSVC ) # MSVC mangles symbols differently, and
               # PrintFunctionNames.export contains C++ symbols.
  if( NOT LLVM_REQUIRES_RTTI )
    if( NOT LLVM_REQUIRES_EH )
      set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/PrintFunctionNames.exports)
    endif()
  endif()
endif()

add_llvm_library(PrintFunctionNames MODULE PrintFunctionNames.cpp PLUGIN_TOOL clang)

if(LLVM_ENABLE_PLUGINS AND (WIN32 OR CYGWIN))
  set(LLVM_LINK_COMPONENTS
    Support
  )
  clang_target_link_libraries(PrintFunctionNames PRIVATE
    clangAST
    clangBasic
    clangFrontend
    )
endif()
