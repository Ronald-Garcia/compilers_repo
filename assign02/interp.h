#ifndef INTERP_H
#define INTERP_H

#include "value.h"
#include "string.h"
#include "environment.h"
class Node;
class Location;

class Interpreter {
private:
  Node *m_ast;
  
public:
  Interpreter(Node *ast_to_adopt);
  ~Interpreter();

  void analyze();
  Value execute();

private:
  // TODO: private member functions
  void analyze_recurse(Node* cur_ast_node, Environment* env);
  Value execute_recurse(Node* cur_ast_node, Environment* env);
  static Value intrinsic_print(Value args[], unsigned num_args,  const Location &loc, Interpreter* interp);
  static Value intrinsic_println(Value args[], unsigned num_args,  const Location &loc, Interpreter* interp);
  static Value intrinsic_readint(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_mkarr(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_len(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_get(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_set(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_push(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_pop(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);

  static Value intrinsic_strcat(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_substr(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);
  static Value intrinsic_strlen(Value args[], unsigned num_args, const Location &loc, Interpreter* interp);

};

#endif // INTERP_H
