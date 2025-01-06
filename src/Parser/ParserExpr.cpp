#include "alert.h"

#include "TypeInfo.h"
#include "Object.h"
#include "Token.h"
#include "Node.h"

#include "Parser.h"
#include "Error.h"

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
  auto tok = this->cur;

  auto nd = this->p_if_expr();

  nd->first_tok = tok;

  auto op = this->cur;

  if (this->eat(Op::Assign)) {
    nd = Node::new_node(ND_Assign, op, nd, this->p_assign());
  }

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

  nd->last_tok = this->cur->prev;

  return nd;
}

Node* Parser::p_if_expr() {
  auto tok = this->cur;

  auto nd = this->p_range();

  nd->first_tok = tok;

  if (auto op = this->cur; this->eat(Kwd::If)) {
    auto x = Node::new_node(ND_ExprIf, op, nullptr);

    x->nd_if_then = nd;
    x->nd_if_cond = this->p_expr();

    if (this->eat(Kwd::Else))
      x->nd_if_else = this->p_range();

    nd = x;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

Node* Parser::p_range() {
  auto tok = this->cur;

  auto nd = this->p_logical();

  nd->first_tok = tok;

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Punct::Ellipsis))
      nd = Node::new_node(ND_Range, op, nd, this->p_logical());
    else
      break;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// logical ::=
//   bit_calc (("&&" | "||") bit_calc)*
//
Node* Parser::p_logical() {
  auto tok = this->cur;

  auto nd = this->p_in();

  nd->first_tok = tok;

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Kwd::And))
      nd = Node::new_node(ND_And, op, nd, this->p_in());
    else if (this->eat(Kwd::Or))
      nd = Node::new_node(ND_Or, op, nd, this->p_in());
    else
      break;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

Node* Parser::p_in() {
  auto tok = this->cur;

  auto nd = this->p_bit_calc();

  nd->first_tok = tok;

  if (auto op = this->cur; this->eat(Kwd::In)) {
    nd = Node::new_node(ND_In, op, nd, this->p_bit_calc());
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

Node* Parser::p_bit_calc() {
  auto tok = this->cur;

  auto nd = this->p_equality();

  nd->first_tok = tok;

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

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// equality ::=
//   compare (("==" | "!=") compare)*
//
Node* Parser::p_equality() {
  auto tok = this->cur;

  auto nd = this->p_compare();

  nd->first_tok = tok;

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

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// compare ::=
//   shift (("<" | ">" | "<=" | ">=") shift)*
//
Node* Parser::p_compare() {
  auto tok = this->cur;

  auto nd = this->p_shift();

  nd->first_tok = tok;

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

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// shift ::=
//   add (("<<" | ">>") add)*
//
Node* Parser::p_shift() {
  auto tok = this->cur;

  auto nd = this->p_add();

  nd->first_tok = tok;

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::LShift))
      nd = Node::new_node(ND_LShift, op, nd, this->p_add());
    else if (this->eat(Op::RShift))
      nd = Node::new_node(ND_RShift, op, nd, this->p_add());
    else
      break;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// add ::=
//   mul (("+" | "-") mul)*
//
Node* Parser::p_add() {
  auto tok = this->cur;

  auto nd = this->p_mul();

  nd->first_tok = tok;

  while (this->check()) {
    auto op = this->cur;

    if (this->eat(Op::Add))
      nd = Node::new_node(ND_Add, op, nd, this->p_mul());
    else if (this->eat(Op::Sub))
      nd = Node::new_node(ND_Sub, op, nd, this->p_mul());
    else
      break;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// mul ::=
//   factor (("*" | "/" | "%") factor)*
//
Node* Parser::p_mul() {
  auto tok = this->cur;

  auto nd = this->p_unary();

  nd->first_tok = tok;

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

  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// unary ::=
//   ("++" | "--" | "+" | "-" | "not" | "ref") p_subscript ("++" | "--")?
//
Node* Parser::p_unary() {
  auto tok = this->cur;

  Node* nd = nullptr;

  //
  // forward plus
  //
  if (this->eat(Op::Add)) {
    nd = expect_expr(p_subscript);
  }

  //
  // forward minus
  //   -A --> 0 - A
  //
  else if (this->eat(Op::Sub))
    nd = Node::new_node(ND_Sub, tok, Parser::new_zero(), this->p_subscript());

  //
  // not
  //
  else if (this->eat(Kwd::Not))
    nd = Node::new_node(ND_Not, tok, this->p_subscript(), nullptr);

  //
  // ref
  //
  else if (this->eat(Kwd::Ref))
    nd = Node::new_node(ND_Ref, tok, this->p_subscript(), nullptr);

  //
  // cast
  //
  else if (this->eat(Kwd::Cast)) {
    this->expect_template_args_open();

    auto cast_to = this->p_expect_type();

    this->expect_template_args_close();

    this->expect_brace_open();

    auto from = this->p_expr();

    this->expect_brace_close();

    nd = Node::new_node(ND_Cast, tok);

    nd->nd_cast_to_type = cast_to;
    nd->nd_cast_from_expr = from;
  }

  else
    nd = this->p_subscript();

  nd->first_tok = tok;
  nd->last_tok = this->cur->prev;

  return nd;
}

// ------------
// subscript ::=
//   p_scope_resol ("[" expr "]" | "." unary | "(" expr ("," expr)* ")")*
//
Node* Parser::p_subscript() {
  auto tok = this->cur;

  auto nd = this->p_scope_resol();

  nd->first_tok = tok;

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
    // member access
    //
    else if (this->eat(Op::MemberAccess)) {
      nd = Node::new_node(ND_MemberAccess, op, nd, this->p_scope_resol());
    }

    //
    // Call func
    else if (this->eat_brace_open()) {
      auto cf = Node::new_node(ND_CallFunc, op, nullptr);

      cf->nd_callfunc_callee = nd;

      // A.B()
      //  --> B(A)  (replaced in Sema)
      if (nd->is(ND_MemberAccess)) {
        cf->nd_callfunc_is_method_call = true;

        cf->nd_callfunc_method_self = nd->nd_lhs;

        cf->nd_callfunc_callee = nd->nd_rhs;

        cf->nd_callfunc_args.insert(cf->nd_callfunc_args.begin(),
                                    cf->nd_callfunc_method_self);
      }

      if (!this->eat_brace_close()) {
        do {
          cf->append(this->p_expr());
        } while (this->eat_comma());

        this->expect_brace_close();
      }

      nd = cf;
    }

    else
      break;
  }

  nd->last_tok = this->cur->prev;

  return nd;
}

//
// scope_resol ::=
//   ident ("::" ident)*
//
Node* Parser::p_scope_resol() {
  auto tok = this->cur;
  auto nd = this->p_factor();

  nd->first_tok = tok;

  if (this->match(TokenPunctKind::ScopeResol)) {
    if (!nd->is(ND_Identifier))
      Error(this->cur, "invalid syntax").crash();

    auto sr = Node::new_node(ND_ScopeResol, this->cur, nullptr);

    sr->nd_scope_resol_first = nd;

    while (this->eat(Punct::ScopeResol)) {
      sr->append(this->p_expect_identifier(true));
    }

    nd = sr;
  }

  //
  // call constructor with initializer list
  // A{ ... }
  if (auto keep = this->cur; nd->is_id_or_sr() && this->eat(Punct::BlockBraceOpen)) {
    try {
      auto callctor = Node::new_node(ND_CallConstructor, this->cur);

      callctor->nd_callctor_ctor_side = nd;

      // empty init-list is not valid, but may be block of any statement
      if (this->match(Punct::BlockBraceClose)) {
        this->cur = keep;
        goto _end_parse_callctor;
      }

      do {
        auto pair = Node::new_node(ND_CallCtorPair, this->cur);

        pair->nd_callctor_init_key = this->expect_ident();

        this->expect_colon();

        pair->nd_callctor_init_value = this->p_expr();

        callctor->append(pair);
      } while (this->eat(Punct::Comma));

      this->expect(Punct::BlockBraceClose);

      nd = callctor;
    }
    catch (Error const& e) {
      this->cur = keep;
    }

  _end_parse_callctor:;
  }

  nd->last_tok = this->cur->prev;

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

    arr->first_tok = tok;

    do {
      arr->append(this->p_expr());
    } while (this->eat(Punct::Comma));

    this->expect(Punct::ArrayBraceClose);

    arr->last_tok = this->cur->prev;

    return arr;
  }

  //
  // expr (wrapped by brace)
  if (this->eat(Punct::BraceOpen)) {
    auto nd = this->p_expr();

    nd->first_tok = tok;

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

    nd->last_tok = this->cur->prev;

    return nd;
  }

  //
  // immidiately dict
  //   dict ::= "{" pair ("," pair)* "}"
  //   pair ::= expr ":" expr
  if (this->eat(Punct::BlockBraceOpen)) {
    auto dict = Node::new_node(ND_Dict, tok, nullptr);

    dict->first_tok = tok;

    //
    // don't parse empty dict (may be block of statement)
    if (this->match(Punct::BlockBraceClose)) {
      this->cur = tok;
      goto _pass_dict;
    }

    do {
      // make pair
      auto dict_pair = Node::new_node(ND_DictPair, tok, nullptr);

      dict_pair->nd_dict_pair_key = this->p_expr();

      this->expect(Punct::Colon);
      dict_pair->nd_dict_pair_value = this->p_expr();

      dict->append(dict_pair);
    } while (this->eat(Punct::Comma));

    this->expect(Punct::BlockBraceClose);

    dict->last_tok = this->cur->prev;

    return dict;
  _pass_dict:;
  }

  Node* nd = nullptr;

  if (this->eat(Kwd::True))
    nd = Node::new_value(tok, ObjBool::make(true));

  else if (this->eat(Kwd::False))
    nd = Node::new_value(tok, ObjBool::make(false));

  else if (this->match(TokenKind::Identifier)) {
    nd = this->p_expect_identifier(true);
  }

  else if ((nd = this->p_literal())) {
    this->next();
  }

  else
    Error(this->cur, "invalid syntax").crash();

  nd->first_tok = tok;
  nd->last_tok = this->cur->prev;

  return nd;
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
