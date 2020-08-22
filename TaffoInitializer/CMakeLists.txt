add_llvm_library(TaffoInitializer OBJECT BUILDTREE_ONLY
  TaffoInitializerPass.cpp
  Annotations.cpp
  AnnotationParser.cpp

  ADDITIONAL_HEADERS
  AnnotationParser.h
  TaffoInitializerPass.h
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_property(TARGET obj.TaffoInitializer APPEND_STRING PROPERTY
    LINK_FLAGS " -undefined dynamic_lookup")
endif()

set_property(TARGET obj.TaffoInitializer PROPERTY CXX_STANDARD 11)
set_property(TARGET obj.TaffoInitializer PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET obj.TaffoInitializer PROPERTY POSITION_INDEPENDENT_CODE ON)
