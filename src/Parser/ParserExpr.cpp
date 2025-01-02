#include "alert.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Parser.h"

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
      nd = Node::new_compare(CMP_BiggerOrEqual, op, nd, this->p_shift());
    else if (this->eat(Op::RightBigOrEq))
      nd = Node::new_compare(CMP_BiggerOrEqual, op, this->p_shift(), nd);
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

  if (this->match(TokenPunctKind::ScopeResol)) {
    if (!nd->is(ND_Identifier))
      Error(this->cur, "invalid syntax").crash();

    auto sr = Node::new_node(ND_ScopeResol, this->cur, nullptr);

    sr->nd_scope_resol_first = nd;

    while (this->eat(Punct::ScopeResol)) {
      sr->append(this->p_expect_identifier(true));
    }

    return sr;
  }

  return nd;
}

//
// factor ::=
//   literal | ident | "(" expr ")"
//
Node* Parser::p_factor() {

  auto tok = this->cur;

  //
  // raw array
  if (this->eat(Punct::ArrayBraceOpen)) {
    auto arr = Node::new_node(ND_Array, tok, nullptr);

    do {
      arr->append(this->p_expr());
    } while (this->eat(Punct::Comma));

    this->expect(Punct::ArrayBraceClose);

    return arr;
  }

  //
  // expr (wrapped by brace)
  if (this->eat(Punct::BraceOpen)) {
    auto nd = this->p_expr();

    //
    // if eat comma, parse as raw tuple
    if (this->eat(Punct::Comma)) {
      auto tuple = Node::new_node(ND_Tuple, tok, nullptr);

      tuple->append(nd);

      do {
        tuple->append(this->p_expr());
      } while (this->eat(Punct::Comma));

      nd = tuple;
    }

    this->expect(Punct::BraceClose);

    return nd;
  }

  //
  // immidiately dict
  //   dict ::= "{" pair ("," pair)* "}"
  //   pair ::= expr ":" expr
  if (this->eat(Punct::BlockBraceOpen)) {
    auto dict = Node::new_node(ND_Dict, tok, nullptr);

    do {
      // make pair
      auto dict_pair = Node::new_node(ND_DictPair, tok, nullptr);

      dict_pair->nd_dict_pair_key = this->p_expr();

      this->expect(Punct::Colon);
      dict_pair->nd_dict_pair_value = this->p_expr();

      dict->append(dict_pair);
    } while (this->eat(Punct::Comma));

    this->expect(Punct::BlockBraceClose);

    return dict;
  }

  if (this->eat(Kwd::True))
    return Node::new_value(this->cur, ObjBool::make(true));

  if (this->eat(Kwd::False))
    return Node::new_value(this->cur, ObjBool::make(false));

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
