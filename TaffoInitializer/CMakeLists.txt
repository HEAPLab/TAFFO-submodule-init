add_llvm_library(TaffoInitializer OBJECT BUILDTREE_ONLY
  TaffoInitializerPass.cpp
  Annotations.cpp
  AnnotationParser.cpp

  ADDITIONAL_HEADERS
  AnnotationParser.h
  TaffoInitializerPass.h
)

set_property(TARGET obj.TaffoInitializer PROPERTY POSITION_INDEPENDENT_CODE ON)
