#include <cassert>
#include <algorithm>
#include <memory>
#include "ast.h"
#include "environment.h"
#include "node.h"
#include "exceptions.h"
#include "function.h"
#include "interp.h"

Interpreter::Interpreter(Node *ast_to_adopt)
  : m_ast(ast_to_adopt) {
}

Interpreter::~Interpreter() {
  delete m_ast;
}

void Interpreter::analyze() {
  Environment env;

  analyze_recurse(m_ast, env);
  // implement
}

void Interpreter::analyze_recurse(Node* cur_ast_node, Environment& env) {
  int cur_tag = cur_ast_node->get_tag();

  switch (cur_tag) {

    case AST_UNIT:
    case AST_STATEMENT:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++ ) {
        analyze_recurse(*i, env);
      }
      break;

    case AST_VARDEF:

      if (env.define_variable(cur_ast_node->get_kid(0)->get_str())) {
        SemanticError::raise(cur_ast_node->get_loc(), "Reference %s already defined", cur_ast_node->get_str().c_str());
      }
      analyze_recurse(cur_ast_node->get_kid(0), env);
      break;
    case AST_VARREF:
      if (!(env.get_variable(cur_ast_node->get_str()))) {
        SemanticError::raise(cur_ast_node->get_loc(), "Undefined reference to %s.", cur_ast_node->get_str().c_str());
      }
      return;
      break;
    case AST_INT_LITERAL:
      return;

    default:
    {
      auto lhs = cur_ast_node->get_kid(0);
      auto rhs = cur_ast_node->get_last_kid();

      analyze_recurse(lhs, env);
      analyze_recurse(rhs, env);
    }    
  }

}

Value Interpreter::execute() {
  Value result;

  auto cur_node = m_ast;
  Environment env;

  result = execute_recurse(cur_node, env);

  return result;
}

Value Interpreter::execute_recurse(Node* cur_ast_node, Environment& env) {

  int cur_tag = cur_ast_node->get_tag();

  switch(cur_tag) {
    case AST_UNIT:
    case AST_STATEMENT:
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
      return *env.get_variable(cur_ast_node->get_str());
    case AST_VARDEF:
      env.define_variable(cur_ast_node->get_kid(0)->get_str());
      return Value(0);
    case AST_ASSIGN:
    {
      auto lhs = cur_ast_node->get_kid(0);
      auto rhs = cur_ast_node->get_kid(1);

      auto rhs_val = execute_recurse(rhs, env);

      env.assign_variable(lhs->get_str(), rhs_val);

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

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() == 0 || val2.get_ival() == 0) ? 0 : 1;
        
        return Value(ret_val); 
      }
    case AST_LOGICAL_OR:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() == 0 && val2.get_ival() == 0) ? 0 : 1;
        
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
    default:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        auto ret_val = (val1.get_ival() != val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
    }

    
  }

}

