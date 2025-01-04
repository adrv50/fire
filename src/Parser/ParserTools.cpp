#include "alert.h"
#include "Object.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

bool Parser::check() {
  return !this->cur->is(TokenKind::End);
}

Token* Parser::next() {
  auto tok = this->cur;

  this->cur = this->cur->next;

  return tok;
}

Token* Parser::insert(Token* tok) {
  return this->cur->insert(tok);
}

bool Parser::match(TokenKind k) {
  return this->cur->is(k);
}

bool Parser::match(TokenPunctKind k) {
  return this->cur->is_punct(k);
}

bool Parser::match(TokenOperatorKind k) {
  return this->cur->is_op(k);
}

bool Parser::match(TokenKwdKind k) {
  return this->cur->is_kwd(k);
}

bool Parser::eat(TokenKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenPunctKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenOperatorKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

bool Parser::eat(TokenKwdKind k) {
  if (this->match(k)) {
    this->save();
    this->next();
    return true;
  }

  return false;
}

Token* Parser::expect(TokenKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected " + Token::kind_to_str(k) + " but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenPunctKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::punct_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenOperatorKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::op_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Token* Parser::expect(TokenKwdKind k) {
  if (!this->eat(k))
    Error(this->cur,
          "expected '" + Token::kwd_to_str(k) + "' but found '" + this->cur->str + "'")
        .crash();

  return this->cur->prev;
}

Node* Parser::new_zero() {
  return Node::new_value(nullptr, ObjInt::make(0));
}

//
// node_assign_with_op:
//   wrapper for create a new node of assign-expr with operator
//
// A op # "=" B
//   --> A = (A op B)
//
Node* Parser::new_assign_with_op(NodeKind kind, Token* tok, Node* lhs, Node* rhs) {
  return Node::new_node(ND_Assign, tok, lhs, Node::new_node(kind, tok, lhs, rhs));
}

bool Parser::eat_ident() {
  return this->eat(TokenKind::Identifier);
}

Token* Parser::expect_ident() {
  return this->expect(TokenKind::Identifier);
}

// -----------------
//  Parser::eat_semi
// ----------------------------------
bool Parser::eat_semi() {
  return this->eat(TokenKind::Semi);
}

// -----------------
//  Parser::eat_colon
// ----------------------------------
bool Parser::eat_colon() {
  return this->eat(TokenPunctKind::Colon);
}

// -----------------
//  Parser::eat_comma
// ----------------------------------
bool Parser::eat_comma() {
  return this->eat(TokenPunctKind::Comma);
}

// -----------------
//  Parser::eat_brace_open
// ----------------------------------
bool Parser::eat_brace_open() {
  return this->eat(TokenPunctKind::BraceOpen);
}

// -----------------
//  Parser::eat_brace_close
// ----------------------------------
bool Parser::eat_brace_close() {
  return this->eat(TokenPunctKind::BraceClose);
}

// -----------------
//  Parser::expect_semi
// ----------------------------------
Token* Parser::expect_semi() {
  return this->expect(TokenKind::Semi);
}

// -----------------
//  Parser::expect_colon
// ----------------------------------
Token* Parser::expect_colon() {
  return this->expect(TokenPunctKind::Colon);
}

// -----------------
//  Parser::expect_comma
// ----------------------------------
Token* Parser::expect_comma() {
  return this->expect(TokenPunctKind::Comma);
}

// -----------------
//  Parser::expect_brace_open
// ----------------------------------
Token* Parser::expect_brace_open() {
  return this->expect(TokenPunctKind::BraceOpen);
}

// -----------------
//  Parser::expect_brace_close
// ----------------------------------
Token* Parser::expect_brace_close() {
  return this->expect(TokenPunctKind::BraceClose);
}

// -----------------
//  Parser::expect_block_open
// ----------------------------------
Token* Parser::expect_block_open() {
  return this->expect(TokenPunctKind::BlockBraceOpen);
}

// -----------------
//  Parser::expect_block_close
// ----------------------------------
Token* Parser::expect_block_close() {
  return this->expect(TokenPunctKind::BlockBraceClose);
}

// -----------------
//  Parser::eat_template_args_open
// ----------------------------------
bool Parser::eat_template_args_open() {
  return this->eat(TokenPunctKind::AngleBraceOpen);
}

// -----------------
//  Parser::eat_template_args_close
// ----------------------------------
bool Parser::eat_template_args_close() {
  if (this->match(Op::RShift)) {
    this->cur->set_punct(Punct::AngleBraceClose);

    this->insert(Token::make(TokenKind::Punctuator)->set_punct(Punct::AngleBraceClose));
  }

  return this->eat(Punct::AngleBraceClose);
}

// -----------------
//  Parser::expect_template_args_open
// ----------------------------------
Token* Parser::expect_template_args_open() {
  if (!this->eat_template_args_open())
    Error(this->cur, "expected '<' but found '" + this->cur->str + "'").crash();

  return this->cur->prev;
}

// -----------------
//  Parser::expect_template_args_close
// ----------------------------------
Token* Parser::expect_template_args_close() {
  if (!this->eat_template_args_close())
    Error(this->cur, "expected '>' but found '" + this->cur->str + "'").crash();

  return this->cur->prev;
}
