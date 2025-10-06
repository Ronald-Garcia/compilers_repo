#ifndef STRING_H
#define STRING_H

#include <vector>
#include <string>
#include "valrep.h"

class String : public ValRep {
private:
  std::string text;

  // value semantics prohibited
  String(const String &);
  String &operator=(const String &);

public:
  String(std::string text);
  virtual ~String();

  std::string get_text() const { return text; }

  int len() { return text.size(); }

  std::string substr(int ind_1, int ind_2);
};

#endif // STRING_H
