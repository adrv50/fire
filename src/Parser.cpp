#include "Parser.h"
#include "Error.h"

#include "alert.h"

using Kwd = TokenKwdKind;
using Punct = TokenPunctKind;
using Op = TokenOperatorKind;

//
// program ::=
//   root*
//
Node* Parser::parse() {
  auto node = Node::new_node(ND_Program, this->cur);

  while (this->check())
    node->append(this->p_root());

  return node;
}

//
// root ::=
//   let | func
//
Node* Parser::p_root() {
  if (auto node = this->p_let(); node)
    return node;

  else if ((node = this->p_func()))
    return node;

  Error(this->cur, "expected function or variable declaration").crash();
}

//
// func ::=
//   func ident "(" func_args ("," func_args)* ")" ("->" type)? block
//
Node* Parser::p_func() {
  if (this->eat(Kwd::Func)) {
    auto node = Node::new_node(ND_Function, this->cur);

    node->nd_func_name = this->expect_ident();

    this->expect(Punct::BraceOpen);

    if (!this->eat(Punct::BraceClose)) {
      do {
        node->append(this->p_func_arg());
      } while (this->eat(Punct::Comma));

      this->expect(Punct::BraceClose);
    }

    if (this->eat(Punct::ResultTypeSpecifier)) {
      node->nd_func_result_type = this->p_expect_type();
    }

    node->nd_func_body = this->p_block(true);

    return node;
  }

  return nullptr;
}

//
// func_arg ::=
//   ident ":" type
//
Node* Parser::p_func_arg() {
  auto node = Node::new_node(ND_FunctionArg, this->cur);

  node->nd_func_arg_name = this->expect_ident();

  this->expect(Punct::Colon);

  node->nd_func_arg_type = this->p_expect_type();

  return node;
}

//
// stmt ::=
//   block | let | func |
//   (return expr? | break | continue) semi |
//   expr semi
//
Node* Parser::p_stmt() {
  //
  // block
  //
  if (auto node = this->p_block())
    return node;

  //
  // let
  //
  else if ((node = this->p_let()))
    return node;

  //
  // func
  //
  else if ((node = this->p_func()))
    return node;

  //
  // loop
  //
  else if ((node = this->p_loop()))
    return node;

  //
  // return
  //
  else if (this->eat(Kwd::Return)) {
    auto node = Node::new_node(ND_Return, this->cur);

    if (!this->eat_semi()) {
      node->nd_return_expr = this->p_expr();
      this->expect_semi();
    }

    return node;
  }

  //
  // break
  //
  else if (this->eat(Kwd::Break)) {
    auto node = Node::new_node(ND_Break, this->cur);
    this->expect_semi();
    return node;
  }

  //
  // continue
  //
  else if (this->eat(Kwd::Continue)) {
    auto node = Node::new_node(ND_Continue, this->cur);
    this->expect_semi();
    return node;
  }

  auto expr = this->p_expr();

  this->expect_semi();

  return expr;
}

//
// let ::=
//   let ident (":" type)? ("=" expr)? semi
//
Node* Parser::p_let() {

  if (this->eat(Kwd::Let)) {
    auto node = Node::new_node(ND_Let, this->cur);

    node->nd_let_name = this->expect_ident();

    if (this->eat(Punct::Colon)) {
      node->nd_let_type = this->p_expect_type();
    }

    if (this->eat(Op::Assign)) {
      node->nd_let_init = this->p_expr();
    }

    this->expect_semi();

    return node;
  }

  return nullptr;
}

//
// loop ::=
//   loop block
//
Node* Parser::p_loop() {
  if (this->eat(Kwd::Loop)) {
    auto node = Node::new_node(ND_Loop, this->cur);

    node->nd_loop_body = this->p_block();

    return node;
  }

  return nullptr;
}

//
// block ::=
//   "{" stmt* "}"
//
Node* Parser::p_block(bool expected) {
  if ((expected && this->expect(Punct::BlockBraceOpen)) ||
      this->eat(Punct::BlockBraceOpen)) {
    auto node = Node::new_node(ND_Block, this->cur);

    while (!this->match(Punct::BlockBraceClose))
      node->append(this->p_stmt());

    this->expect(Punct::BlockBraceClose);

    return node;
  }

  return nullptr;
}

// ------------
// expr ::=
//   assign
//
Node* Parser::p_expr() {
  return this->p_assign();
}

// ------------
// assign ::=
//   add (("=" | "+=" | "-=" | "*=" | "/=" | "%=") add)*
//
Node* Parser::p_assign() {
  auto nd = this->p_logical();

  auto op = this->cur;

  if (this->eat(Op::Assign))
    nd->nd_rhs = this->p_assign();

  else if (this->eat(Op::AddAssign))
    nd = Parser::new_assign_with_op(ND_Add, op, nd, this->p_assign());

  else if (this->eat(Op::SubAssign))
    nd = Parser::new_assign_with_op(ND_Sub, op, nd, this->p_assign());

  else if (this->eat(Op::MulAssign))
    nd = Parser::new_assign_with_op(ND_Mul, op, nd, this->p_assign());

  else if (this->eat(Op::DivAssign))
    nd = Parser::new_assign_with_op(ND_Div, op, nd, this->p_assign());

  else if (this->eat(Op::ModAssign))
    nd = Parser::new_assign_with_op(ND_Mod, op, nd, this->p_assign());

  return nd;
}

// ------------
// logical ::=
//   bit_calc (("&&" | "||") bit_calc)*
//
Node* Parser::p_logical() {
  auto nd = this->p_bit_calc();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Kwd::And))
      nd = Node::new_node(ND_And, op, nd, this->p_bit_calc());
    else if (this->eat(Kwd::Or))
      nd = Node::new_node(ND_Or, op, nd, this->p_bit_calc());
    else
      break;
  }

  return nd;
}

Node* Parser::p_bit_calc() {
  auto nd = this->p_equality();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::BitAnd))
      nd = Node::new_node(ND_BitAnd, op, nd, this->p_equality());
    else if (this->eat(Op::BitOr))
      nd = Node::new_node(ND_BitOr, op, nd, this->p_equality());
    else if (this->eat(Op::BitXor))
      nd = Node::new_node(ND_BitXor, op, nd, this->p_equality());
    else
      break;
  }

  return nd;
}

// ------------
// equality ::=
//   compare (("==" | "!=") compare)*
//
Node* Parser::p_equality() {

  auto nd = this->p_compare();

  while (this->check()) {
    auto op = this->cur;

    // "=="
    if (this->eat(Op::Equal))
      nd = Node::new_node(ND_Equal, op, nd, this->p_compare());

    // "!="
    //  --> !(a == b)
    else if (this->eat(Op::NotEqual))
      nd =
          Node::new_node(ND_Not, op, Node::new_node(ND_Equal, op, nd, this->p_compare()));

    else
      break;
  }

  return nd;
}

// ------------
// compare ::=
//   shift (("<" | ">" | "<=" | ">=") shift)*
//
Node* Parser::p_compare() {
  auto nd = this->p_shift();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::LeftBig))
      nd = Node::new_compare(CMP_Bigger, op, nd, this->p_shift());
    else if (this->eat(Op::RightBig))
      nd = Node::new_compare(CMP_Bigger, op, this->p_shift(), nd);
    if (this->eat(Op::LeftBigOrEq))
      nd = Node::new_compare(CMP_BigggerOrEqual, op, nd, this->p_shift());
    else if (this->eat(Op::RightBigOrEq))
      nd = Node::new_compare(CMP_BigggerOrEqual, op, this->p_shift(), nd);
    else
      break;
  }

  return nd;
}

// ------------
// shift ::=
//   add (("<<" | ">>") add)*
//
Node* Parser::p_shift() {
  auto nd = this->p_add();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::LShift))
      nd = Node::new_node(ND_LShift, op, nd, this->p_add());
    else if (this->eat(Op::RShift))
      nd = Node::new_node(ND_RShift, op, nd, this->p_add());
    else
      break;
  }

  return nd;
}

// ------------
// add ::=
//   mul (("+" | "-") mul)*
//
Node* Parser::p_add() {
  auto nd = this->p_mul();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::Add))
      nd = Node::new_node(ND_Add, op, nd, this->p_mul());
    else if (this->eat(Op::Sub))
      nd = Node::new_node(ND_Sub, op, nd, this->p_mul());
    else
      break;
  }

  return nd;
}

// ------------
// mul ::=
//   factor (("*" | "/" | "%") factor)*
//
Node* Parser::p_mul() {
  auto nd = this->p_unary();

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::Mul))
      nd = Node::new_node(ND_Mul, op, nd, this->p_unary());
    else if (this->eat(Op::Div))
      nd = Node::new_node(ND_Div, op, nd, this->p_unary());
    else if (this->eat(Op::Mod))
      nd = Node::new_node(ND_Mod, op, nd, this->p_unary());
    else
      break;
  }

  return nd;
}

// ------------
// unary ::=
//   ("+" | "-" | "not" | "ref") factor
//
Node* Parser::p_unary() {
  auto op = this->cur;

  //
  // forward plus
  //
  if (this->eat(Op::Add))
    return this->p_subscript();

  //
  // forward minus
  //   -A --> 0 - A
  //
  else if (this->eat(Op::Sub))
    return Node::new_node(ND_Sub, op, Parser::new_zero(), this->p_subscript());

  //
  // not
  //
  else if (this->eat(Kwd::Not))
    return Node::new_node(ND_Not, op, this->p_subscript(), nullptr);

  //
  // ref
  //
  else if (this->eat(Kwd::Ref))
    return Node::new_node(ND_Ref, op, this->p_subscript(), nullptr);

  return this->p_subscript();
}

// ------------
// subscript ::=
//   unary ("[" expr "]" | "." unary | "(" expr ("," expr)* ")")*
//
Node* Parser::p_subscript() {
  auto nd = this->p_call_func();

  while (this->check()) {
    auto op = this->cur;

    //
    // subscript
    //
    if (this->eat(Punct::ArrayBraceOpen)) {
      nd = Node::new_node(ND_Subscript, op, nd, this->p_expr());
      this->expect(Punct::ArrayBraceClose);
    }

    //
    // member access (or method call)
    //
    else if (this->eat(Op::MemberAccess)) {
      auto rhs = this->p_call_func();

      // if rhs is call function, set method call flag
      // and left side is use for "self"
      if (rhs->is(ND_CallFunc)) {
        rhs->nd_callfunc_is_method_call = true;
        rhs->nd_callfunc_method_self = nd;

        nd = rhs;
      }
      else {
        nd = Node::new_node(ND_MemberAccess, op, nd, rhs);
      }
    }

    else
      break;
  }

  return nd;
}

//
// call_func ::=
//   ident "(" expr ("," expr)* ")"
//
Node* Parser::p_call_func() {
  auto nd = this->p_scope_resol();

  if (auto tok = this->cur; this->eat(Punct::BraceOpen)) {
    if (!nd->is(ND_Identifier) && !nd->is(ND_ScopeResol))
      Error(tok, "invalid syntax").crash();

    auto cf = Node::new_node(ND_CallFunc, tok, nullptr);

    cf->nd_callfunc_callee = nd;

    if (!this->eat(Punct::BraceClose)) {
      do {
        cf->append(this->p_expr());
      } while (this->eat(Punct::Comma));

      this->expect(Punct::BraceClose);
    }

    return cf;
  }

  return nd;
}

//
// scope_resol ::=
//   ident ("::" ident)*
//
Node* Parser::p_scope_resol() {
  auto nd = this->p_factor();

  if (this->match(TokenPunctKind::ScopeResol) && !nd->is(ND_Identifier))
    Error(this->cur, "invalid syntax").crash();

  for (Token* op; (op = this->cur), this->eat(Punct::ScopeResol);)
    nd = Node::new_node(ND_ScopeResol, op, nd, this->p_factor());

  return nd;
}

//
// factor ::=
//   literal | ident | "(" expr ")"
//
Node* Parser::p_factor() {
  if (this->eat(Punct::BraceOpen)) {
    auto nd = this->p_expr();
    this->expect(Punct::BraceClose);
    return nd;
  }

  if (this->match(TokenKind::Identifier)) {
    return this->p_expect_identifier(true);
  }

  if (auto nd = this->p_literal()) {
    this->next();
    return nd;
  }

  Error(this->cur, "invalid syntax").crash();
}

Node* Parser::p_literal() {
  auto tok = this->cur;

  switch (tok->kind) {
  case TokenKind::Decimal:
    return Node::new_value(tok, ObjInt::make(tok->literal_data.v_int));

  case TokenKind::Hexadecimal:
    return Node::new_value(tok, ObjInt::make(tok->literal_data.v_hex));

  case TokenKind::Binary:
    return Node::new_value(tok, ObjInt::make(tok->literal_data.v_bin));

  case TokenKind::Float:
    return Node::new_value(tok, ObjFloat::make(tok->literal_data.v_float));

  case TokenKind::Boolean:
    return Node::new_value(tok, ObjBool::make(tok->literal_data.v_bool));

  case TokenKind::Character:
    return Node::new_value(tok, ObjChar::make(tok->literal_data.v_char));

  case TokenKind::String:
    return Node::new_value(tok, ObjStr::make(tok->v_str));
  }

  return nullptr;
}

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

bool Parser::eat_ident(bool allow_kwd) {
  return this->eat(TokenKind::Identifier) || (allow_kwd && this->eat(TokenKind::Keyword));
}

Token* Parser::expect_ident(bool allow_kwd) {
  if (allow_kwd && this->match(TokenKind::Keyword))
    return this->next();

  return this->expect(TokenKind::Identifier);
}

bool Parser::eat_semi() {
  return this->eat(TokenKind::Semi);
}

Token* Parser::expect_semi() {
  return this->expect(TokenKind::Semi);
}

bool Parser::eat_template_args_open() {
  return this->eat(Punct::BeginTemplateArgs) && this->expect(Punct::AngleBraceOpen);
}

bool Parser::eat_template_args_close() {
  if (this->match(Op::RShift)) {
    this->cur->set_punct(Punct::AngleBraceClose);

    this->insert(Token::make(TokenKind::Punctuator)->set_punct(Punct::AngleBraceClose));
  }

  return this->eat(Punct::AngleBraceClose);
}

Token* Parser::expect_template_args_open() {
  return this->expect(Punct::AngleBraceOpen);
}

Token* Parser::expect_template_args_close() {
  return this->expect(Punct::AngleBraceClose);
}

Node* Parser::p_expect_type() {
  auto node = Node::new_node(ND_TypeName, this->expect_ident(true));

  if (this->eat_template_args_open()) {
    do {
      node->append(this->p_expect_type());
    } while (this->eat(Punct::Comma));

    this->expect_template_args_close();
  }

  return node;
}

Node* Parser::p_expect_identifier(bool allow_qualifier) {

  auto node = Node::new_node(ND_Identifier, this->expect_ident(true));

  if (allow_qualifier)
    this->p_parse_id_qualifier(node);

  return node;
}

void Parser::p_parse_id_qualifier(Node* nd) {
  if (!this->eat(Punct::BeginTemplateArgs))
    return;

  this->expect_template_args_open();

  do {
    nd->append(this->p_expect_type());
  } while (this->eat(Punct::Comma));

  this->expect_template_args_close();
}
