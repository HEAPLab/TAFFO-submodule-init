add_llvm_library(TaffoInitializer MODULE
  TaffoInitializerPass.cpp
  TaffoInitializerPass.h
  Annotations.cpp
  AnnotationParser.cpp
  AnnotationParser.h
  
  PLUGIN_TOOL
  opt
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_property(TARGET TaffoInitializer APPEND_STRING PROPERTY
    LINK_FLAGS " -undefined dynamic_lookup")
endif()

set_property(TARGET TaffoInitializer PROPERTY CXX_STANDARD 11)
set_property(TARGET TaffoInitializer PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(TaffoInitializer TaffoUtils)
