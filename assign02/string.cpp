
#include "string.h"
#include "valrep.h"

String::String(std::string text)
  : ValRep(VALREP_STRING),
  text(text) {

}

String::~String() {

}

std::string String::substr(int ind_1, int size) {

  if (ind_1 + size > len()) {
    return "";
  }

  return text.substr(ind_1, size);
}