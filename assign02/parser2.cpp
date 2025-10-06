#include <cassert>
#include <map>
#include <string>
#include <memory>
#include "token.h"
#include "ast.h"
#include "exceptions.h"
#include "parser2.h"

////////////////////////////////////////////////////////////////////////
// Parser2 implementation
// This version of the parser builds an AST directly,
// rather than first building a parse tree.
////////////////////////////////////////////////////////////////////////

// This is the grammar (Unit is the start symbol):
//
// Unit -> Stmt
// Unit -> Stmt Unit
// Stmt -> E ;
// E -> T E'
// E' -> + T E'
// E' -> - T E'
// E' -> epsilon
// T -> F T'
// T' -> * F T'
// T' -> / F T'
// T' -> epsilon
// F -> number
// F -> ident
// F -> ( E )

Parser2::Parser2(Lexer *lexer_to_adopt)
  : m_lexer(lexer_to_adopt)
  , m_next(nullptr) {
}

Parser2::~Parser2() {
  delete m_lexer;
}

Node *Parser2::parse() {
  return parse_Unit();
}
Node *Parser2::parse_Unit() {
  // note that this function produces a "flattened" representation
  // of the unit

  std::unique_ptr<Node> unit(new Node(AST_UNIT));
  for (;;) {
    unit->append_kid(parse_TStmt());
    if (m_lexer->peek() == nullptr)
      break;
  }

  return unit.release();
}

Node *Parser2::parse_Stmt() {
  // Stmt -> ^ E ;

  std::unique_ptr<Node> s(new Node(AST_STATEMENT));

  Node *next_tok = m_lexer->peek();
  if (next_tok == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for statement");
  }

  int next_tag = next_tok->get_tag();

  // Stmt → var ident ;
  if (next_tag == TOK_VARIABLE) {
    std::unique_ptr<Node> vardef(new Node(AST_VARDEF));
    std::unique_ptr<Node> vardunitef_lexer(expect(static_cast<enum TokenKind>(TOK_VARIABLE)));
    std::unique_ptr<Node> name(expect(static_cast<enum TokenKind>(TOK_IDENTIFIER)));
    name->set_tag(AST_VARREF);
    vardef->append_kid(name.release());
    s->append_kid(vardef.release());

    expect_and_discard(TOK_SEMICOLON);
  } else if (next_tag == TOK_IF) {
    // Stmt →       if ( A ) { SList }                     -- if stmt
    // Stmt →       if ( A ) { SList } else { SList }      -- if/else stmt 

    
    std::unique_ptr<Node> if_statement(new Node(AST_IF));

    expect_and_discard(TOK_IF);
    expect_and_discard(TOK_LPAREN);

    if_statement->append_kid(parse_A());

    expect_and_discard(TOK_RPAREN);

    expect_and_discard(TOK_LBRACE);

    if_statement->append_kid(parse_SList());

    expect_and_discard(TOK_RBRACE);

    Node* else_check = m_lexer->peek();

    if (else_check != nullptr && else_check->get_tag() == TOK_ELSE) {
      expect_and_discard(TOK_ELSE);
      expect_and_discard(TOK_LBRACE);
      if_statement->append_kid(parse_SList());
      expect_and_discard(TOK_RBRACE);
    }

    s->append_kid(if_statement.release());
  } else if (next_tag == TOK_WHILE) {
  // Stmt →       while ( A ) { SList }                  -- while loop

    expect_and_discard(TOK_WHILE);
    std::unique_ptr<Node> while_statement(new Node(AST_WHILE));

    expect_and_discard(TOK_LPAREN);
    while_statement->append_kid(parse_A());

    expect_and_discard(TOK_RPAREN);

    expect_and_discard(TOK_LBRACE);
    while_statement->append_kid(parse_SList());
    expect_and_discard(TOK_RBRACE);
    s->append_kid(while_statement.release());
  } else {
        s->append_kid(parse_A());
        expect_and_discard(TOK_SEMICOLON);
  }




  return s.release();
}

Node *Parser2::parse_TStmt() { 

  Node *next_tok = m_lexer->peek();

  if (next_tok == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for statement");
  }

  int next_tag = next_tok->get_tag();
  

  // TStmt -> Func
  if (next_tag == TOK_FUNCTION) {
    expect_and_discard(TOK_FUNCTION);

    return parse_Func();
  }
    


  return parse_Stmt();
}

Node *Parser2::parse_Func() {

  // Func -> function ident ( OptPList ) { SList }
  std::unique_ptr<Node> func_node(new Node(AST_FUNC));

  std::unique_ptr<Node> ident(expect(static_cast<enum TokenKind>(TOK_IDENTIFIER)));

  if (ident == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for identifier."); 
  }

  ident->set_tag(AST_VARREF);

  func_node->append_kid(ident.release());
  expect_and_discard(TOK_LPAREN);

  auto plist = parse_OptPList();

  if (plist != nullptr) {
    func_node->append_kid(plist);
  }

  expect_and_discard(TOK_RPAREN);

  expect_and_discard(TOK_LBRACE);
  func_node->append_kid(parse_SList());
  expect_and_discard(TOK_RBRACE);

  return func_node.release();
}

Node *Parser2::parse_OptPList() {


  Node *next_tok = m_lexer->peek();

  if (next_tok== nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for PList or RBRACE"); 
  }

  int next_tag = next_tok->get_tag();


  // OptPList -> epsilon
  if (next_tag == TOK_RPAREN) {
    return nullptr;
  } 

  // OptPList -> PList

  return parse_PList();

}


Node *Parser2::parse_OptArgList() {


  Node *next_tok = m_lexer->peek();

  if (next_tok== nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for ArgList or RPAREN"); 
  }

  int next_tag = next_tok->get_tag();


  // OptArgList -> epsilon
  if (next_tag == TOK_RPAREN) {
    return nullptr;
  } 

  // OptArgList -> ArgList

  return parse_ArgList();
}

Node *Parser2::parse_ArgList() {

  std::unique_ptr<Node> arglist(new Node(AST_ARGLIST));

  Node *next_tok = m_lexer->peek();
  if (next_tok == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for L or ArgList"); 
  }  


  // PList -> ident , PList
  for (;;) {
    std::unique_ptr<Node> L(parse_L());

    arglist->append_kid(L.release());
    Node *cur_next_tok = m_lexer->peek();
    if (cur_next_tok == nullptr ) {
      SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for identifier or PList"); 
    }

    int cur_next_tag = cur_next_tok->get_tag();

    if (cur_next_tag != TOK_COMMA) {
      break;
    }
    expect_and_discard(TOK_COMMA);
  }
  return arglist.release();
}

Node *Parser2::parse_SList() {

  // SList -> Stmt
  // SList -> Stmt SList
  std::unique_ptr<Node> slist(new Node(AST_STATEMENT_LIST));
  for (;;) {
    slist->append_kid(parse_Stmt());
    if (m_lexer->peek()->get_tag() == TOK_RBRACE)
      break;
  }

  return slist.release();

}

Node *Parser2::parse_PList() {

  std::unique_ptr<Node> plist(new Node(AST_PARAMETER_LIST));

  Node *next_tok = m_lexer->peek();
  if (next_tok == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for identifier or PList"); 
  }  


  // PList -> ident , PList
  for (;;) {
    std::unique_ptr<Node> ident(expect(static_cast<enum TokenKind>(TOK_IDENTIFIER)));

    ident->set_tag(AST_VARREF);

    plist->append_kid(ident.release());
    Node *cur_next_tok = m_lexer->peek();
    if (cur_next_tok == nullptr ) {
      SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input looking for identifier or PList"); 
    }

    int cur_next_tag = cur_next_tok->get_tag();

    if (cur_next_tag != TOK_COMMA) {

      break;
    }
    expect_and_discard(TOK_COMMA);
  }
  return plist.release();
}

Node *Parser2::parse_E() {
  // E -> ^ T E'

  // Get the AST corresponding to the term (T)
  Node *ast = parse_T();

  // Recursively continue the additive expression
  return parse_EPrime(ast);
}

// This function is passed the "current" portion of the AST
// that has been built so far for the additive expression.
Node *Parser2::parse_EPrime(Node *ast_) {
  // E' -> ^ + T E'
  // E' -> ^ - T E'
  // E' -> ^ epsilon

  std::unique_ptr<Node> ast(ast_);

  // peek at next token
  Node *next_tok = m_lexer->peek();
  if (next_tok != nullptr) {
    int next_tok_tag = next_tok->get_tag();
    if (next_tok_tag == TOK_PLUS || next_tok_tag == TOK_MINUS)  {
      // E' -> ^ + T E'
      // E' -> ^ - T E'
      std::unique_ptr<Node> op(expect(static_cast<enum TokenKind>(next_tok_tag)));

      // build AST for next term, incorporate into current AST
      Node *term_ast = parse_T();
      ast.reset(new Node(next_tok_tag == TOK_PLUS ? AST_ADD : AST_SUB, {ast.release(), term_ast}));

      // copy source information from operator node
      ast->set_loc(op->get_loc());

      // continue recursively
      return parse_EPrime(ast.release());
    }
  }

  // E' -> ^ epsilon
  // No more additive operators, so just return the completed AST
  return ast.release();
}

Node *Parser2::parse_T() {
  // T -> F T'

  // Parse primary expression
  Node *ast = parse_F();

  // Recursively continue the multiplicative expression
  return parse_TPrime(ast);
}

Node *Parser2::parse_TPrime(Node *ast_) {
  // T' -> ^ * F T'
  // T' -> ^ / F T'
  // T' -> ^ epsilon

  std::unique_ptr<Node> ast(ast_);

  // peek at next token
  Node *next_tok = m_lexer->peek();
  if (next_tok != nullptr) {
    int next_tok_tag = next_tok->get_tag();
    if (next_tok_tag == TOK_TIMES || next_tok_tag == TOK_DIVIDE)  {
      // T' -> ^ * F T'
      // T' -> ^ / F T'
      std::unique_ptr<Node> op(expect(static_cast<enum TokenKind>(next_tok_tag)));

      // build AST for next primary expression, incorporate into current AST
      Node *primary_ast = parse_F();
      ast.reset(new Node(next_tok_tag == TOK_TIMES ? AST_MULTIPLY : AST_DIVIDE, {ast.release(), primary_ast}));

      // copy source information from operator node
      ast->set_loc(op->get_loc());

      // continue recursively
      return parse_TPrime(ast.release());
    }
  }

  // T' -> ^ epsilon
  // No more multiplicative operators, so just return the completed AST
  return ast.release();
}

Node *Parser2::parse_F() {
  // F -> ^ number
  // F -> ^ ident
  // F -> ^ string
  // F -> ^ ( E )

  Node *next_tok = m_lexer->peek();
  if (next_tok == nullptr) {
    error_at_current_loc("Unexpected end of input looking for primary expression");
  }

  int tag = next_tok->get_tag();
  if (tag == TOK_INTEGER_LITERAL || tag == TOK_IDENTIFIER) {
    // F -> ^ number
    // F -> ^ ident
    std::unique_ptr<Node> tok(expect(static_cast<enum TokenKind>(tag)));
    int ast_tag = tag == TOK_INTEGER_LITERAL ? AST_INT_LITERAL : AST_VARREF;
    std::unique_ptr<Node> ast(new Node(ast_tag));

    Node* cur_next_node = m_lexer->peek();

    if (cur_next_node == nullptr) {
      error_at_current_loc("Unexpected end of input looking for primary expression");
    }

    if (cur_next_node->get_tag() == TOK_LPAREN) {
      expect_and_discard(TOK_LPAREN);

      std::unique_ptr<Node> func_call(new Node(AST_FNCALL));
      ast->set_str(tok->get_str());
      ast->set_loc(tok->get_loc());

      func_call->append_kid(ast.release());

      auto opt_arg_list = std::unique_ptr<Node>(parse_OptArgList());

      if (opt_arg_list.get() != nullptr) {
        func_call->append_kid(opt_arg_list.release());

      }


      expect_and_discard(TOK_RPAREN);
      return func_call.release();
    }
    ast->set_str(tok->get_str());
    ast->set_loc(tok->get_loc());
    return ast.release();

  } else if (tag == TOK_LPAREN) {
    // F -> ^ ( E )
    expect_and_discard(TOK_LPAREN);
    std::unique_ptr<Node> ast(parse_A());
    expect_and_discard(TOK_RPAREN);
    return ast.release();
  } else if (tag == TOK_STRING) {
    std::unique_ptr<Node> string = std::unique_ptr<Node>(expect(static_cast<enum TokenKind>(TOK_STRING)));

    std::unique_ptr<Node> ast_string = std::unique_ptr<Node>(new Node(AST_STRING));

    ast_string->set_loc(string->get_loc());
    ast_string->set_str(string->get_str());

    return ast_string.release();
  }
  else {
    SyntaxError::raise(next_tok->get_loc(), "Invalid primary expression");
  }
}

Node *Parser2::expect(enum TokenKind tok_kind) {
  std::unique_ptr<Node> next_terminal(m_lexer->next());
  if (next_terminal->get_tag() != tok_kind) {
    SyntaxError::raise(next_terminal->get_loc(), "Unexpected token '%s'", next_terminal->get_str().c_str());
  }
  return next_terminal.release();
}

void Parser2::expect_and_discard(enum TokenKind tok_kind) {
  Node *tok = expect(tok_kind);
  delete tok;
}

void Parser2::error_at_current_loc(const std::string &msg) {
  SyntaxError::raise(m_lexer->get_current_loc(), "%s", msg.c_str());
}


Node *Parser2::parse_A() {
  Node *next_tok = m_lexer->peek(1);
  Node *next_next_tok = m_lexer->peek(2);
  
  if (next_tok == nullptr) {
    SyntaxError::raise(m_lexer->get_current_loc(), "Unexpected end of input.");
  }

  // A    → ident = A
  if (next_next_tok && next_tok->get_tag() == TOK_IDENTIFIER && next_next_tok->get_tag() == TOK_ASSIGNMENT) {
    std::unique_ptr<Node> ident(expect(static_cast<enum TokenKind>(TOK_IDENTIFIER)));
    std::unique_ptr<Node> assign(expect(static_cast<enum TokenKind>(TOK_ASSIGNMENT)));
    auto loc = assign->get_loc();
    std::unique_ptr<Node> val(parse_A());

    ident->set_tag(AST_VARREF);

    assign.reset(new Node(AST_ASSIGN, {ident.release(), val.release()}));
    assign->set_loc(loc);

    return assign.release();
  }

  // A    → L

  std::unique_ptr<Node> L(parse_L());

  return L.release();
}


Node *Parser2::parse_L() {


  std::unique_ptr<Node> r(parse_R());

  Node *next_tok = m_lexer->peek();

  // L    → R
  if (next_tok == nullptr) {
    return r.release();
  }


  int next_tag = next_tok->get_tag();

  if (next_tag != TOK_OR && next_tag != TOK_AND) {

    return r.release();
    
  }

  int ast_tag = next_tag == TOK_OR ? AST_LOGICAL_OR : AST_LOGICAL_AND;

  // L    → R || R
  // L    → R && R

  std::unique_ptr<Node> logical_operation(expect(static_cast<enum TokenKind>(next_tag)));

  std::unique_ptr<Node> operand(parse_R());

  r.reset(new Node(ast_tag, {r.release(), operand.release()}));

  r->set_loc(logical_operation->get_loc());

  

  return r.release();
}

Node *Parser2::parse_R() {


  std::unique_ptr<Node> e(parse_E());

  Node* next_tok = m_lexer->peek();

  // R    → E
  if (next_tok == nullptr) {
    return e.release();
  }


  int next_tag = next_tok->get_tag();


  int ast_tag;

  
  // R    → E < E
  // R    → E <= E
  // R    → E > E
  // R    → E >= E
  // R    → E == E
  // R    → E != E

  switch(next_tag) {
    case TOK_LESS_THAN: ast_tag = AST_LT; break;
    case TOK_LESS_THAN_EQUAL: ast_tag = AST_LTE; break;
    case TOK_GREATER_THAN: ast_tag = AST_GT; break;
    case TOK_GREATER_THAN_EQUAL: ast_tag = AST_GTE; break;
    case TOK_EQUAL: ast_tag = AST_EQ; break;
    case TOK_NOT_EQUAL: ast_tag = AST_NOT_EQ; break;
    default: return e.release();    
  }
  
 
  std::unique_ptr<Node> operand(expect(static_cast<enum TokenKind>(next_tag)));
  std::unique_ptr<Node> other(parse_E());

  e.reset(new Node(ast_tag, {e.release(), other.release()}));

  e->set_loc(operand->get_loc());


  return e.release();
}