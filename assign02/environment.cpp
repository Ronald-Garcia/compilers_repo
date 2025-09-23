#include "environment.h"
#include "exceptions.h"
#include "value.h"
#include <csignal>
#include <memory>

Environment::Environment(Environment *parent)
  : m_parent(parent)
  , m_vars() {
  assert(m_parent != this);
}

Environment::~Environment() {
}

int Environment::define_variable(std::string name) {
  int check = m_vars.count(name);

  if (check) {
    return -1;
  }


  m_vars.emplace(name, Value(0));
  return 0;
}
  
Value* Environment::get_variable(std::string name) {
  int check = m_vars.count(name);
  if (!check) {
    return nullptr;
  }
  return &m_vars.at(name);
}

int Environment::assign_variable(std::string name, Value& val) {

  int check = m_vars.count(name);

  if (!check) {
    return -1;
  }
  m_vars[name] = val;
  return 0;
}

// implement member functions
