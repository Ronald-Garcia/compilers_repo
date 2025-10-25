// Copyright (c) 2021-2024, David H. Hovemeyer <david.hovemeyer@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <map>
#include "grammar_symbols.h"
#include "parse.tab.h"
#include "node.h"
#include "ast.h"
#include "exceptions.h"
#include "semantic_analysis.h"

SemanticAnalysis::SemanticAnalysis(const Options &options)
  : m_options(options)
  , m_global_symtab(new SymbolTable(nullptr, "global")) {
  m_cur_symtab = m_global_symtab;
  m_all_symtabs.push_back(m_global_symtab);
}

SemanticAnalysis::~SemanticAnalysis() {
  // The semantic analyzer owns the SymbolTables and their Symbols,
  // so delete them. Note that the AST has pointers to Symbol objects,
  // so the SemanticAnalysis object should live at least as long
  // as the AST.
  for (auto i = m_all_symtabs.begin(); i != m_all_symtabs.end(); ++i)
    delete *i;
}

void SemanticAnalysis::visit_struct_type(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_union_type(Node *n) {
  RuntimeError::raise("union types aren't supported");
}

// DONE FOR NOW
void SemanticAnalysis::visit_variable_declaration(Node *n) {

  int cur_tag = n->get_tag();
  std::shared_ptr<Type> symbolType;

  if (cur_tag != AST_VARIABLE_DECLARATION) {
    SemanticError::raise(n->get_loc(), "Unexpected node.");
  }

  // auto unspecified_storage_node = n->get_kid(0);

  auto type_node = n->get_kid(1);
  auto declarator_list_node = n->get_kid(2);
  visit_basic_type(type_node);

  auto var_type = type_node->get_type();

  for (auto i = declarator_list_node->cbegin(); i != declarator_list_node->cend(); i++) {
    traverse_declarator_list(*i, var_type);

  }
}

// DONE FOR NOW
void SemanticAnalysis::visit_basic_type(Node *n) {
  // TODO: implement

  int cur_tag = n->get_tag();

  if (cur_tag != AST_BASIC_TYPE) {
    SemanticError::raise(n->get_loc(), "Invalid type found.");
  }

  if (n->get_num_kids() == 0) {
    SemanticError::raise(n->get_loc(), "Invalid type found.");
  } 

  // DEFINITION:          


  bool intCount = false;
  bool charCount = false;
  bool voidCount = false;
  bool unsignedCount = false;
  bool signedCount = false;
  bool longCount = false;
  bool shortCount = false;

  uint8_t constVolatile = 0b0000;
  
  for (auto i = n->cbegin(); i != n->cend(); i++) {

    int child_tag = (*i)->get_tag();

    switch(child_tag) {
      case TOK_INT:

        // check if only one has existed
        if (intCount || charCount || voidCount) {
          SemanticError::raise((*i)->get_loc(), "Extra `int` type.");
        }
        intCount = true;
        break;
      case TOK_CHAR:
      
        // long and short don't work with char.
        if (longCount || shortCount) {
          SemanticError::raise((*i)->get_loc(), "`long` or `short` can only be used with `int`.");
        } else if ((intCount || charCount || voidCount) ) {
          // check if only one has existed
          SemanticError::raise((*i)->get_loc(), "Extra `char` type.");
        }
        charCount = true;
        break;
      case TOK_VOID:
        
        // long, short, unsigned, and signed dont work with void.
        if (longCount || shortCount) {
          SemanticError::raise((*i)->get_loc(), "`long` or `short` can only be used with `int`.");
        } else if (unsignedCount|| signedCount) {
          SemanticError::raise((*i)->get_loc(), "`void` should have no other types.");
        } else if (intCount || charCount || voidCount) {
          // check if only one has existed
          SemanticError::raise((*i)->get_loc(), "Extra `void` type.");
        }
        voidCount = true;
        break;
      case TOK_UNSIGNED:
        if (signedCount) {
          SemanticError::raise((*i)->get_loc(), "`signed` and `unsigned` are mutually exclusive.");
        } else if (voidCount) {
          SemanticError::raise((*i)->get_loc(), "`void` should have no other types.");
        } else if (unsignedCount) {
          SemanticError::raise((*i)->get_loc(), "Extra `unsigned` type.");
        }
        unsignedCount = true;
        break;
      case TOK_SIGNED:
        if (unsignedCount) {
          SemanticError::raise((*i)->get_loc(), "`signed` and `unsigned` are mutually exclusive.");
        } else if (voidCount) {
          SemanticError::raise((*i)->get_loc(), "`void` should have no other types.");
        } else if (signedCount) {
          SemanticError::raise((*i)->get_loc(), "Extra `signed` type.");
        }
        signedCount = true;
        break;
      case TOK_LONG:
        if (shortCount) {
          SemanticError::raise((*i)->get_loc(), "`short` and `long` are mutually exclusive.");
        } else if (voidCount) {
          SemanticError::raise((*i)->get_loc(), "`void` should have no other types.");
        } else if (longCount) {
          SemanticError::raise((*i)->get_loc(), "Extra `long` type.");
        }
        longCount = true;
        break;
      case TOK_CONST:
        if (constVolatile & 0b0100) {
          SemanticError::raise((*i)->get_loc(), "Extra `const` qualifier.");
        }
        if (constVolatile == 0) {
          constVolatile |= 0b1100;
        } else {
          constVolatile |= 0b0100;
        }
        break;
      case TOK_VOLATILE:
        if (constVolatile & 0b0001) {
          SemanticError::raise((*i)->get_loc(), "Extra `const` qualifier.");
        }
        if (constVolatile == 0) {
          constVolatile |= 0b0011;
        } else {
          constVolatile |= 0b0001;
        }
        break;
      default:
        if (longCount != 0) {
          SemanticError::raise((*i)->get_loc(), "`short` and `long` are mutually exclusive.");
        } else if (voidCount != 0) {
          SemanticError::raise((*i)->get_loc(), "`void` should have no other qualifiers.");
        } else if (shortCount != 0) {
          SemanticError::raise((*i)->get_loc(), "Extra `short` qualifier.");
        }
        shortCount = true;
        break;
    }
  }

  BasicTypeKind typeKind;

  typeKind=BasicTypeKind::INT;

  if (longCount) {
    typeKind=BasicTypeKind::LONG;
  } else if (shortCount) {
    typeKind=BasicTypeKind::SHORT;
  }
  
  if (charCount) {
    typeKind = BasicTypeKind::CHAR;
  }

  if (voidCount) {
    typeKind = BasicTypeKind::VOID;
  }

  bool isSigned = (!signedCount && !unsignedCount) || signedCount;

  auto basicType = std::shared_ptr<BasicType>(new BasicType(typeKind, isSigned));


  switch(constVolatile) {
    case 0b0011:
      n->set_type(std::shared_ptr<QualifiedType>(new QualifiedType(basicType, TypeQualifier::VOLATILE)));  
      break;
    case 0b1100:
      n->set_type(std::shared_ptr<QualifiedType>(new QualifiedType(basicType, TypeQualifier::CONST)));
      break;
    case 0b1101: {
      auto temp = std::shared_ptr<QualifiedType>(new QualifiedType(basicType, TypeQualifier::CONST));
      n->set_type(std::shared_ptr<QualifiedType>(new QualifiedType(temp, TypeQualifier::VOLATILE)));
      break;
    }
    case 0b1011: {
      auto temp = std::shared_ptr<QualifiedType>(new QualifiedType(basicType, TypeQualifier::VOLATILE));
      n->set_type(std::shared_ptr<QualifiedType>(new QualifiedType(temp, TypeQualifier::CONST)));
      break;
    }
    default: 
      n->set_type(basicType);
      break;    
  }

}


// UNUSED
void SemanticAnalysis::visit_named_declarator(Node *n) {
  // TODO: implement
}

// UNUSED
void SemanticAnalysis::visit_pointer_declarator(Node *n) {
  // TODO: implement
}

// UNUSED
void SemanticAnalysis::visit_array_declarator(Node *n) {
  // TODO: implement
}

// DONE FOR NOW
void SemanticAnalysis::visit_function_definition(Node *n) {
  // TODO: implement

  int cur_tag = n->get_tag();

  if (cur_tag != AST_FUNCTION_DEFINITION) {
    SemanticError::raise(n->get_loc(), "Unexpected node.");
  }

  auto type_node = n->get_kid(0);
  auto name_node = n->get_kid(1);
  auto param_list_node = n->get_kid(2);
  auto statement_list_node = n->get_kid(3);


  visit_basic_type(type_node);

  auto func_type = type_node->get_type();
  auto func_name = name_node->get_str();

  m_cur_symtab->add_entry(n->get_loc(), SymbolKind::FUNCTION, func_name, func_type);
  enter_scope(func_name);
  visit_function_parameter_list(param_list_node);

  visit_statement_list(statement_list_node);
}

void SemanticAnalysis::visit_function_declaration(Node *n) {
  // TODO: implement

  int cur_tag = n->get_tag();

  if (cur_tag != AST_FUNCTION_DECLARATION) {
    SemanticError::raise(n->get_loc(), "Unexpected token.");
  }

  auto base_type_node = n->get_kid(0);

  auto name_node = n->get_kid(1);

  auto param_list_node = n->get_kid(2);


  visit_basic_type(base_type_node);

  auto base_type = base_type_node->get_type();

  auto name = name_node->get_str();

  

 



}

void SemanticAnalysis::visit_function_parameter_list(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_function_parameter(Node *n) {
  // TODO: implement
}


void SemanticAnalysis::visit_statement_list(Node *n) {
  // TODO: implement

  int cur_tag = n->get_tag();

  if (cur_tag != AST_STATEMENT_LIST) {
    SemanticError::raise(n->get_loc(), "Unexpected token.");
  }

  for (auto i = n->cbegin(); i != n->cend(); i++) {
    int child_tag = (*i)->get_tag();

    switch(child_tag) {

      case AST_VARIABLE_DECLARATION:
        visit_variable_declaration(*i);
        break;
      default:
        break;


    }
  }
}

void SemanticAnalysis::visit_return_expression_statement(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_struct_type_definition(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_binary_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_unary_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_postfix_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_conditional_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_cast_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_function_call_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_field_ref_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_indirect_field_ref_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_array_element_ref_expression(Node *n) {
  // TODO: implement
}

void SemanticAnalysis::visit_variable_ref(Node *n) {

  int cur_tag = n->get_tag();

  if (cur_tag != AST_VARIABLE_REF) {
    SemanticError::raise(n->get_loc(), "Unexpected token.");
  }

  m_cur_symtab



  // TODO: implement
}

void SemanticAnalysis::visit_literal_value(Node *n) {
  // TODO: implement
}

SymbolTable *SemanticAnalysis::enter_scope(const std::string &name) {
  SymbolTable *symtab = new SymbolTable(m_cur_symtab, name);
  m_all_symtabs.push_back(symtab);
  m_cur_symtab = symtab;
  return symtab;
}

void SemanticAnalysis::leave_scope() {
  assert(m_cur_symtab->get_parent() != nullptr);
  m_cur_symtab = m_cur_symtab->get_parent();
}

// TODO: implement helper functions

// DONE FOR NOW
void SemanticAnalysis::traverse_declarator_list(Node* root, std::shared_ptr<Type> t) {

  int tag = root->get_tag();

  if (tag == AST_NAMED_DECLARATOR) {

    auto ident = root->get_kid(0);

    std::string name = ident->get_str();
    Location loc = ident->get_loc();
    SymbolKind kind = SymbolKind::VARIABLE;

    m_cur_symtab->add_entry(loc, kind, name, t);
  } else if (tag == AST_ARRAY_DECLARATOR) {

    auto size_node = root->get_last_kid();
    int size = std::stoi(size_node->get_str());
    auto arr_type = std::shared_ptr<ArrayType>(new ArrayType(t, size));

    for (auto i = root->cbegin(); i != root->cend(); i++) {
      traverse_declarator_list(*i, arr_type);
    } 
  } else if (tag == AST_POINTER_DECLARATOR) {
    auto ptr_type = std::shared_ptr<PointerType>(new PointerType(t));

    for (auto i = root->cbegin(); i != root->cend(); i++) {
      traverse_declarator_list(*i, ptr_type);
    }
  }

}