#ifndef TOKEN_H
#define TOKEN_H

// This header file defines the tags used for tokens (i.e., terminal
// symbols in the grammar.)

enum TokenKind {
  TOK_IDENTIFIER,
  TOK_INTEGER_LITERAL,
  TOK_PLUS,
  TOK_MINUS,
  TOK_TIMES,
  TOK_DIVIDE,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_SEMICOLON,
  
  // ASSIGN01
  TOK_ASSIGNMENT,
  TOK_OR,
  TOK_AND,
  TOK_LESS_THAN,
  TOK_LESS_THAN_EQUAL,
  TOK_GREATER_THAN,
  TOK_GREATER_THAN_EQUAL,
  TOK_EQUAL,
  TOK_NOT_EQUAL,
  TOK_VARIABLE,

  // ASSIGN02
  TOK_FUNCTION,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE,

  TOK_LBRACE,
  TOK_RBRACE,
  TOK_COMMA,
  // add members for additional kinds of tokens
};

#endif // TOKEN_H
