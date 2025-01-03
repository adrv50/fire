#pragma once

#include "fire-fwd.h"

class Parser {

  using Kwd = TokenKwdKind;
  using Punct = TokenPunctKind;
  using Op = TokenOperatorKind;

  Token* cur;
  Token* ate;

  Token* save() {
    return this->ate = this->cur;
  }

  bool in_repl = false;

public:
  Parser(Token* tok)
      : cur(tok),
        ate(nullptr) {
  }

  void set_in_repl() {
    this->in_repl = true;
  }

  Node* parse();

  Node* p_root();

  Node* p_enum();
  Node* p_def_enumerator();

  Node* p_struct();
  Node* p_struct_member();

  Node* p_class();

  Node* p_namespace();

  Node* p_func();
  Node* p_func_arg();

  Node* p_stmt();
  Node* p_let();
  Node* p_loop();

  Node* p_block(bool expected = false);

  Node* p_expr();

  Node* p_assign();

  // 'and' 'or'
  Node* p_logical();

  // '&' '|' '^'
  Node* p_bit_calc();

  // '==' '!='
  Node* p_equality();

  // '>=' '<=' '>' '<'
  Node* p_compare();

  Node* p_shift();
  Node* p_add();
  Node* p_mul();
  Node* p_unary();
  Node* p_subscript();

  Node* p_scope_resol();
  Node* p_factor();

  Node* p_literal();

private:
  bool check();
  Token* next();

  Token* insert(Token* tok);

  bool match(TokenKind k);
  bool match(TokenPunctKind k);
  bool match(TokenOperatorKind k);
  bool match(TokenKwdKind k);

  bool eat(TokenKind k);
  bool eat(TokenPunctKind k);
  bool eat(TokenOperatorKind k);
  bool eat(TokenKwdKind k);

  Token* expect(TokenKind k);
  Token* expect(TokenPunctKind k);
  Token* expect(TokenOperatorKind k);
  Token* expect(TokenKwdKind k);

  // node create wrapper
  static Node* new_zero();
  static Node* new_assign_with_op(NodeKind kind, Token* tok, Node* lhs, Node* rhs);

  // ident
  bool eat_ident();
  Token* expect_ident();

  // semicolon
  bool eat_semi();
  Token* expect_semi();

  // template args
  bool eat_template_args_open();
  bool eat_template_args_close();
  Token* expect_template_args_open();
  Token* expect_template_args_close();

  // type name
  Node* p_expect_type();

  // identifier (with qualifier)
  Node* p_expect_identifier(bool allow_qualifier = false);
  void p_parse_id_qualifier(Node* nd);
};
