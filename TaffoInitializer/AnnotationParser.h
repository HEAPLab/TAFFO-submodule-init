#include <string>
#include "TaffoInitializerPass.h"
#include "InputInfo.h"


#ifndef __ANNOTATION_PARSER_H__
#define __ANNOTATION_PARSER_H__


namespace taffo {


class AnnotationParser {
  void reset();
  
public:
  std::string target;
  bool isTarget;
  bool backtracking;
  std::shared_ptr<mdutils::MDInfo> metadata;
  
  bool parseAnnotationString(llvm::StringRef annString);
  llvm::StringRef lastError();
};


}


#endif // __ANNOTATION_PARSER_H__
