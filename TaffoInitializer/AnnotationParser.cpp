#include <sstream>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "AnnotationParser.h"


using namespace llvm;
using namespace taffo;
using namespace mdutils;


void AnnotationParser::reset()
{
  target = "";
  isTarget = false;
  backtracking = false;
  metadata.reset();
}


bool AnnotationParser::parseAnnotationString(StringRef annstr)
{
  reset();
  
  std::istringstream strstm(annstr.substr(0, annstr.size()-1));

  bool readNumBits = true;
  std::string head;
  strstm >> head;
  if (head.find("target:") == 0) {
    target = head.substr(7); // strlen("target:") == 7
    strstm >> head;
    isTarget = true;
  }
  if (head == "no_float" || head == "force_no_float") {
    if (head == "no_float")
      backtracking = false;
    else
      backtracking = true;
    strstm >> head;
  }
  if (head == "range")
    readNumBits = false;
  else
    return false;
  
  mdutils::InputInfo *info = new mdutils::InputInfo();
  metadata.reset(info);

  if (readNumBits) {
    int intbits, fracbits;
    strstm >> intbits >> fracbits;
    if (!strstm.fail()) {
      std::string signedflg;
      strstm >> signedflg;
      if (!strstm.fail() && signedflg == "unsigned") {
        info->IType.reset(new mdutils::FPType(intbits + fracbits, fracbits, false));
      } else {
        info->IType.reset(new mdutils::FPType(intbits + fracbits, fracbits, true));
      }
    }
  }

  // Look for Range info
  double Min, Max;
  strstm >> Min >> Max;
  if (!strstm.fail()) {
    info->IRange.reset(new mdutils::Range(Min, Max));
    DEBUG(dbgs() << "Range found: [" << Min << ", " << Max << "]\n");

    // Look for initial error
    double Error;
    strstm >> Error;
    if (!strstm.fail()) {
      DEBUG(dbgs() << "Initial error found " << Error << "\n");
      info->IError.reset(new double(Error));
    }
  }
  
  return true;
}


StringRef AnnotationParser::lastError()
{
  return "Syntax Error";
}
