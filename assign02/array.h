#ifndef ARRAY_H
#define ARRAY_H

#include <vector>
#include <string>
#include "valrep.h"
#include "value.h"

class Array : public ValRep {
private:

  std::vector<Value> arr;

  // value semantics prohibited
  Array(const Array &);
  Array &operator=(const Array &);

public:
  Array(std::vector<Value> arr);
  virtual ~Array();

  std::vector<Value> get_array() const { return arr; }
  void push_val(Value val) { arr.push_back(val); }
  void set_val(Value val, int ind);
  void pop_val() { arr.pop_back(); }
  int len() { return arr.size(); }
  Value get_val(int ind) { return arr.at(ind); }
};

#endif // ARRAY_H
