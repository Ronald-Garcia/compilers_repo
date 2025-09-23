#include <cassert>
#include <algorithm>
#include <memory>
#include <unistd.h>
#include "ast.h"
#include "environment.h"
#include "node.h"
#include "exceptions.h"
#include "function.h"
#include "value.h"
#include "interp.h"

Interpreter::Interpreter(Node *ast_to_adopt)
  : m_ast(ast_to_adopt) {
}

Interpreter::~Interpreter() {
  delete m_ast;
}

void Interpreter::analyze() {
  auto global_env = std::unique_ptr<Environment>(new Environment());

  global_env->bind("print", Value(&intrinsic_print));
  global_env->bind("println", Value(&intrinsic_println));
  global_env->bind("readint", Value(&intrinsic_readint));

  analyze_recurse(m_ast, global_env.release());
  // implement
}

void Interpreter::analyze_recurse(Node* cur_ast_node, Environment* env) {
  int cur_tag = cur_ast_node->get_tag();

  switch (cur_tag) {

    case AST_UNIT:
    case AST_STATEMENT:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++ ) {
        analyze_recurse(*i, env);
      }
      break;

    case AST_FUNC: {

      auto func_env = std::unique_ptr<Environment>(new Environment(env));


      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        analyze_recurse(*i, func_env.get());
      }

    }
      
    case AST_VARDEF:

      if (env->define_variable(cur_ast_node->get_kid(0)->get_str())) {
        SemanticError::raise(cur_ast_node->get_loc(), "Reference %s already defined", cur_ast_node->get_str().c_str());
      }
      analyze_recurse(cur_ast_node->get_kid(0), env);
      break;
    case AST_FNCALL:
      if (!(env->get_variable(cur_ast_node->get_kid(0)->get_str()))) {
        SemanticError::raise(cur_ast_node->get_loc(), "Undefined reference to %s.", cur_ast_node->get_str().c_str());
      }
      return;
    case AST_VARREF:
      if (!(env->get_variable(cur_ast_node->get_str()))) {
        SemanticError::raise(cur_ast_node->get_loc(), "Undefined reference to %s.", cur_ast_node->get_str().c_str());
      }
      return;
    case AST_INT_LITERAL:
      return;
    case AST_IF: {
      auto if_env = std::unique_ptr<Environment>(new Environment(env));
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        analyze_recurse(*i, if_env.get());
      }
      return;
    }

    case AST_WHILE: {
      auto while_env = std::unique_ptr<Environment>(new Environment(env));
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        analyze_recurse(*i, while_env.get());
      }
      return;
    }


    default:
    {
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        analyze_recurse(*i, env);
      }
    }    
  }

}

Value Interpreter::execute() {
  Value result;

  auto cur_node = m_ast;
  auto global_env = std::unique_ptr<Environment>(new Environment());
  global_env->bind("print", Value(&intrinsic_print));
  global_env->bind("println", Value(&intrinsic_println));
  global_env->bind("readint", Value(&intrinsic_readint));

  result = execute_recurse(cur_node, global_env.release());

  return result;
}

Value Interpreter::execute_recurse(Node* cur_ast_node, Environment* env) {

  int cur_tag = cur_ast_node->get_tag();

  switch(cur_tag) {
    case AST_UNIT:
    case AST_STATEMENT:
    case AST_STATEMENT_LIST:
    case AST_PARAMETER_LIST:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++ ) {
        auto next = i + 1;
        if (next == cur_ast_node->cend()) {
          return execute_recurse(*i, env);
        }
        execute_recurse(*i, env); 
      }
      return Value(0);
    case AST_INT_LITERAL:
      return Value(std::stoi(cur_ast_node->get_str()));
    case AST_VARREF:
      return *env->get_variable(cur_ast_node->get_str());
    case AST_VARDEF:
      env->define_variable(cur_ast_node->get_kid(0)->get_str());
      return Value(0);
    case AST_ASSIGN:
    {
      auto lhs = cur_ast_node->get_kid(0);
      auto rhs = cur_ast_node->get_kid(1);

      auto rhs_val = execute_recurse(rhs, env);

      env->assign_variable(lhs->get_str(), rhs_val);

      return Value(rhs_val.get_ival());
    }
      
    case AST_ADD:
      {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        return Value(val1.get_ival() + val2.get_ival()); 
      }
    case AST_SUB:
      {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        return Value(val1.get_ival() - val2.get_ival()); 
      }
    case AST_MULTIPLY:
          {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        return Value(val1.get_ival() * val2.get_ival()); 
      }

    case AST_DIVIDE:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        if (val2.get_ival() == 0) {
          EvaluationError::raise(cur_ast_node->get_loc(), 
          "Divide by zero error.");
        }

        return Value(val1.get_ival() / val2.get_ival()); 
      }
    case AST_LOGICAL_AND:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto ret_val = (execute_recurse(child_1, env).get_ival() && execute_recurse(child_2, env).get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }
    case AST_LOGICAL_OR:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);
        
        auto ret_val = (execute_recurse(child_1, env).get_ival() || execute_recurse(child_2, env).get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }
    case AST_GT:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() > val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_GTE:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() >= val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_LT:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() < val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_LTE:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() <= val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

        case AST_EQ:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() == val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    // AST_NOT_EQ
    case AST_NOT_EQ:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() != val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
    }    
    case AST_IF: {

      auto if_env = std::unique_ptr<Environment>(new Environment(env));

      int num_children = cur_ast_node->get_num_kids();

      auto condition = cur_ast_node->get_kid(0);
      auto true_case = cur_ast_node->get_kid(1);


      auto val_condition = execute_recurse(condition, if_env.get());

      if (val_condition.get_ival()) {
        execute_recurse(true_case, if_env.get());
      } else if (num_children == 3) {

        auto false_case = cur_ast_node->get_kid(2);
        execute_recurse(false_case, if_env.get());
      }

      return Value(0);

    }
    case AST_WHILE: {

      auto while_env = std::unique_ptr<Environment>(new Environment(env));


      auto condition = cur_ast_node->get_kid(0);
      auto true_case = cur_ast_node->get_kid(1);


      auto val_condition = execute_recurse(condition, while_env.get());

      while (val_condition.get_ival()) {
        execute_recurse(true_case, while_env.get());
        val_condition = execute_recurse(condition, while_env.get());
      }

      return Value(0);
    }
    case AST_FNCALL: {

      auto func_name = cur_ast_node->get_kid(0)->get_str();

      auto arglist_node = cur_ast_node->get_kid(1);

      int num_args =arglist_node->get_num_kids(); 
      Value args[num_args];
      int cnt = 0;

      for (auto i = arglist_node->cbegin(); i != arglist_node->cend(); i++) {
        args[cnt++] = execute_recurse(*i, env);
      }

      auto fnc_val = env->get_variable(func_name);

      IntrinsicFn f = fnc_val->get_intrinsic_fn();



      return f(args,num_args, cur_ast_node->get_loc(), this);

    }

    case AST_ARGLIST: {

      return Value(0);

    }


    case AST_FUNC: {
      return Value(0);
        
    }
    default: {
      return Value(0);
    }
  }


}

Value Interpreter::intrinsic_print(
    Value args[], unsigned num_args,
    const Location &loc, Interpreter *interp) {
  if (num_args != 1)
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to print function");
  printf("%s", args[0].as_str().c_str());
  return Value();
}

Value Interpreter::intrinsic_println(
    Value args[], unsigned num_args,
    const Location &loc, Interpreter *interp) {
  if (num_args != 1)
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to print function");
  printf("%s\n", args[0].as_str().c_str());
  return Value();
}

Value Interpreter::intrinsic_readint(
    Value args[], unsigned num_args,
    const Location &loc, Interpreter *interp) {
  int num = 0;
  scanf("%d", &num);
  return Value(num);
}