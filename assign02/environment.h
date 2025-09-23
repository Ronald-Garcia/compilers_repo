#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <cassert>
#include <map>
#include <string>
#include "value.h"

class Environment {
private:
  Environment *m_parent;
  // representation of environment (map of names to values)
  std::map<std::string, Value> m_vars;

  // copy constructor and assignment operator prohibited
  Environment(const Environment &);
  Environment &operator=(const Environment &);

public:
  Environment(Environment *parent = nullptr);
  ~Environment();

  // add member functions allowing lookup, definition, and assignment

  int define_variable(std::string name);
  
  Value* get_variable(std::string name);

  int assign_variable(std::string name, Value& val);

  Value* get_local_variable(std::string name);

  void bind(std::string name, Value val);
  
};

#endif // ENVIRONMENT_H
