#include <cassert>
#include <algorithm>
#include <memory>
#include <unistd.h>
#include "array.h"
#include "ast.h"
#include "environment.h"
#include "node.h"
#include "exceptions.h"
#include "function.h"
#include "value.h"
#include "string.h"
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

  global_env->bind("get", Value(&intrinsic_get));
  global_env->bind("set", Value(&intrinsic_set));
  global_env->bind("mkarr", Value(&intrinsic_mkarr));
  global_env->bind("len", Value(&intrinsic_len));
  global_env->bind("push", Value(&intrinsic_push));
  global_env->bind("pop", Value(&intrinsic_pop));

  global_env->bind("strlen", Value(&intrinsic_strlen));
  global_env->bind("strcat", Value(&intrinsic_strcat));
  global_env->bind("substr", Value(&intrinsic_substr));
  analyze_recurse(m_ast, global_env.release());
  // implement
}

void Interpreter::analyze_recurse(Node* cur_ast_node, Environment* env) {
  int cur_tag = cur_ast_node->get_tag();

  switch (cur_tag) {

    case AST_ARGLIST:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        analyze_recurse(*i, env);
      }
    case AST_UNIT:
    case AST_STATEMENT:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++ ) {
        analyze_recurse(*i, env);
      }
      return;

    case AST_PARAMETER_LIST:
      for (auto i = cur_ast_node->cbegin(); i != cur_ast_node->cend(); i++) {
        env->define_variable((*i)->get_str());
      }
      return;

    case AST_FUNC: {

      env->define_variable(cur_ast_node->get_kid(0)->get_str());

      auto func_env = std::unique_ptr<Environment>(new Environment(env));

      if (cur_ast_node->get_num_kids() == 3) {
        for (auto i = cur_ast_node->get_kid(1)->cbegin() ; i != cur_ast_node->get_kid(1)->cend(); i++) {
          func_env->define_variable((*i)->get_str());
        }
        
      }

      auto func_env_pass = std::unique_ptr<Environment>(new Environment(func_env.release()));

      for (auto i = cur_ast_node->get_last_kid()->cbegin(); i != cur_ast_node->get_last_kid()->cend(); i++) {
        analyze_recurse(*i, func_env_pass.get());
      }
      return;
    }
      
    case AST_VARDEF:

      if (env->define_variable(cur_ast_node->get_kid(0)->get_str())) {
        EvaluationError::raise(cur_ast_node->get_loc(), "Reference %s already defined", cur_ast_node->get_str().c_str());
      }
      analyze_recurse(cur_ast_node->get_kid(0), env);
      return;
    case AST_FNCALL:
      if ((env->get_variable(cur_ast_node->get_kid(0)->get_str()))==nullptr) {
        EvaluationError::raise(cur_ast_node->get_loc(), "Undefined reference to %s.", cur_ast_node->get_str().c_str());
      }
      if (cur_ast_node->get_num_kids() == 2) {
        analyze_recurse(cur_ast_node->get_last_kid(), env);
      }
      return;
    case AST_VARREF:
      if ((env->get_variable(cur_ast_node->get_str()))==nullptr) {
        EvaluationError::raise(cur_ast_node->get_loc(), "Undefined reference to %s.", cur_ast_node->get_str().c_str());
      }
      
      return;
    case AST_STRING:
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
      return;
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

  global_env->bind("get", Value(&intrinsic_get));
  global_env->bind("set", Value(&intrinsic_set));
  global_env->bind("mkarr", Value(&intrinsic_mkarr));
  global_env->bind("len", Value(&intrinsic_len));
  global_env->bind("push", Value(&intrinsic_push));
  global_env->bind("pop", Value(&intrinsic_pop));

  global_env->bind("strlen", Value(&intrinsic_strlen));
  global_env->bind("strcat", Value(&intrinsic_strcat));
  global_env->bind("substr", Value(&intrinsic_substr));

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

          auto cur_val=  execute_recurse(*i, env);
          return cur_val;
        }
        execute_recurse(*i, env); 
      }
      return Value(0);

    case AST_STRING:
      return Value(new String(cur_ast_node->get_str()));
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

      return rhs_val;
    }
      
    case AST_ADD:
      {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        return Value(val1.get_ival() + val2.get_ival()); 
      }
    case AST_SUB:
      {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        return Value(val1.get_ival() - val2.get_ival()); 
      }
    case AST_MULTIPLY:
          {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        return Value(val1.get_ival() * val2.get_ival()); 
      }

    case AST_DIVIDE:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);

        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

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

        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        }

        if (val1.get_ival() == 0) {
          return Value(0);
        }

        auto val2 = execute_recurse(child_2, env);

        if (val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }
        
        return Value(val2.get_ival() != 0); 
      }
    case AST_LOGICAL_OR:
              {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);
        

        auto val1 = execute_recurse(child_1, env);

        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        }

        if (val1.get_ival() == 1) {
          return Value(1);
        }

        auto val2 = execute_recurse(child_2, env);

        if (val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }
        
        return Value(val2.get_ival() != 0); 
      }
    case AST_GT:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        auto ret_val = (val1.get_ival() > val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_GTE:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        auto ret_val = (val1.get_ival() >= val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_LT:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        auto ret_val = (val1.get_ival() < val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

    case AST_LTE:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        auto ret_val = (val1.get_ival() <= val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
      }

        case AST_EQ:
    {
        auto child_1 = cur_ast_node->get_kid(0);
        auto child_2 = cur_ast_node->get_kid(1);

        auto val1 = execute_recurse(child_1, env);
        auto val2 = execute_recurse(child_2, env);
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

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
        if (val1.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_1->get_loc(), "Invalid type.");
        } else if ( val2.get_kind() != VALUE_INT) {
          EvaluationError::raise(child_2->get_loc(), "Invalid type.");
        }

        auto ret_val = (val1.get_ival() != val2.get_ival()) ? 1 : 0;
        
        return Value(ret_val); 
    }    
    case AST_IF: {

      auto if_env = std::unique_ptr<Environment>(new Environment(env));

      int num_children = cur_ast_node->get_num_kids();

      auto condition = cur_ast_node->get_kid(0);
      auto true_case = cur_ast_node->get_kid(1);


      auto val_condition = execute_recurse(condition, if_env.get());

      if (val_condition.get_kind() != VALUE_INT) {
        EvaluationError::raise(condition->get_loc(), "Invalid type.");
      }

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

      if (val_condition.get_kind() != VALUE_INT) {
        EvaluationError::raise(condition->get_loc(), "Invalid type.");
      }

      while (val_condition.get_ival()) {
        execute_recurse(true_case, while_env.get());
        val_condition = execute_recurse(condition, while_env.get());
      }

      return Value(0);
    }
    case AST_FNCALL: {

      auto func_name = cur_ast_node->get_kid(0)->get_str();
      auto fnc_val = env->get_variable(func_name);

      if (fnc_val == nullptr) {
        EvaluationError::raise(cur_ast_node->get_loc(), "Undefined refernence to %s", func_name.c_str());
      }

      int num_kids = cur_ast_node->get_num_kids();

      if (num_kids == 1) {
        
        if (fnc_val->get_kind() == VALUE_FUNCTION) {
          if (fnc_val->get_function()->get_num_params() != 0) {
            EvaluationError::raise(cur_ast_node->get_loc(), "Invalid number of parameters for %s", func_name.c_str());
          }

          auto f = fnc_val->get_function();
          auto func_env = std::unique_ptr<Environment>(new Environment(f->get_parent_env()));      
          return execute_recurse(f->get_body(), func_env.release());
        }
        
        IntrinsicFn f = fnc_val->get_intrinsic_fn();

        int num_args = 0; 
        Value args[num_args];

        return f(args,num_args, cur_ast_node->get_loc(), this);
      }


      auto arglist_node = cur_ast_node->get_kid(1);

      unsigned num_args =arglist_node->get_num_kids(); 

      Value args[num_args];
      int cnt = 0;

      for (auto i = arglist_node->cbegin(); i != arglist_node->cend(); i++) {
        args[cnt++] = execute_recurse(*i, env);
      }



      if (fnc_val->get_kind() == VALUE_FUNCTION) {
        if (num_args != fnc_val->get_function()->get_num_params()) {
          EvaluationError::raise(cur_ast_node->get_loc(), "Invalid number of parameters for %s", func_name.c_str());
        }

        auto f = fnc_val->get_function();
        auto func_env = std::unique_ptr<Environment>(new Environment(f->get_parent_env()));
        for (unsigned i = 0; i < num_args; i++) {
          auto cur_param_name = f->get_params()[i];
          auto cur_param_value = args[i];
          func_env->bind(cur_param_name, cur_param_value);
        }

        auto func_env_pass = std::unique_ptr<Environment>(new Environment(func_env.release()));
        return execute_recurse(f->get_body(), func_env_pass.release());
      }

      IntrinsicFn f = fnc_val->get_intrinsic_fn();



      return f(args,num_args, cur_ast_node->get_loc(), this);

    }
    case AST_FUNC: {

      std::string func_name = cur_ast_node->get_kid(0)->get_str();

      int num_params = cur_ast_node->get_num_kids();
      auto statement_list = cur_ast_node->get_last_kid();

      Node* body = statement_list;
      auto param_names = std::vector<std::string>();

      if (num_params == 3) {
        auto arglist = cur_ast_node->get_kid(1);

        for (auto i = arglist->cbegin(); i != arglist->cend(); ++i) {
          param_names.push_back((*i)->get_str());
        }
      } 

      Value fn_val(new Function(func_name, param_names, env, body));

      env->bind(func_name, fn_val);
      return fn_val;
        
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

Value Interpreter::intrinsic_mkarr(
    Value args[], unsigned num_args,
    const Location &loc, Interpreter *interp) {

  std::vector<Value> arr;

  for (unsigned i = 0; i < num_args; i++) {
    arr.push_back(args[i]);
  }

  auto arr_val = new Array(arr);

  return Value(arr_val);
}

Value Interpreter::intrinsic_len(
    Value args[], unsigned num_args,
    const Location &loc, Interpreter *interp) {

  if (num_args != 1) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to len function");
  }

  if (args[0].get_kind() != VALUE_ARRAY) {
    EvaluationError::raise(
      loc, "Wrong type of arguments passed to len function");

  }

  Array* arr = args[0].get_array();
  int len = arr->len();

  return Value(len);
}

Value Interpreter::intrinsic_get(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 2) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to get function");
  }

  if (args[0].get_kind() != VALUE_ARRAY || args[1].get_kind() != VALUE_INT) {
    EvaluationError::raise(
      loc, "Wrong type of arguments passed to get function");

  }

    Array* arr = args[0].get_array();
    int index = args[1].get_ival();
    
    return arr->get_val(index);

  }

Value Interpreter::intrinsic_set(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 3) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to set function");
  }

    if (args[0].get_kind() != VALUE_ARRAY || args[1].get_kind() != VALUE_INT) {
    EvaluationError::raise(
      loc, "Wrong type of arguments passed to set function");

    }
    Array* arr = args[0].get_array();
    int index = args[1].get_ival();
    Value val = args[2];

    arr->set_val(val, index);
    return val;
  }

Value Interpreter::intrinsic_push(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 2) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to push function");
  }

    if (args[0].get_kind() != VALUE_ARRAY) {
    EvaluationError::raise(
      loc, "Wrong type of argument passed to push function");

    }

    Array* arr = args[0].get_array();
    Value val = args[1];

    arr->push_val(val);
    return val;
  }



  Value Interpreter::intrinsic_pop(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 1) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to pop function");
  }

    if (args[0].get_kind() != VALUE_ARRAY) {
      EvaluationError::raise(
      loc, "Wrong type of argument passed to pop function");
    }

    Array* arr = args[0].get_array();

    if (arr->get_array().size() == 0) {
      EvaluationError::raise(loc, "Array is empty.");
    }

    arr->pop_val();
    
    return Value(0);
  }

  Value Interpreter::intrinsic_strlen(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 1) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to len function");
  }

    if (args[0].get_kind() != VALUE_STRING) {
      EvaluationError::raise(
      loc, "Wrong type of argument passed to len function");
    }

    String* str = args[0].get_string();

    return Value(str->len());
  }


  Value Interpreter::intrinsic_substr(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 3) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to substr function");
  }

    if (args[0].get_kind() != VALUE_STRING || args[1].get_kind() != VALUE_INT || args[2].get_kind() != VALUE_INT) {
      EvaluationError::raise(
      loc, "Wrong type of argument passed to substr function");
    }

    String* str = args[0].get_string();
    int index = args[1].get_ival();
    int num_c = args[2].get_ival();



    return Value(new String(str->substr(index, num_c)));
  }

  
  Value Interpreter::intrinsic_strcat(
  Value args[], unsigned num_args, 
  const Location &loc, Interpreter* interp) {
    if (num_args != 2) {
    EvaluationError::raise(
      loc, "Wrong number of arguments passed to strcat function");
  }

    if (args[0].get_kind() != VALUE_STRING || args[1].get_kind() != VALUE_STRING) {
      EvaluationError::raise(
      loc, "Wrong type of argument passed to strcat function");
    }

    String* str1 = args[0].get_string();
    String* str2 = args[1].get_string();



    return Value(new String(str1->get_text() + str2->get_text()));
  }




  