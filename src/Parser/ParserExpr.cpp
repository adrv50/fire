#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace fire::parser {

static ObjPtr<ObjPrimitive> make_value_from_token(Token const& tok) {
  auto k = tok.kind;
  auto const& s = tok.str;

  auto obj = ObjNew<ObjPrimitive>();

  switch (k) {
  case TokenKind::Int:
    obj->type = TypeKind::Int;
    obj->vi = std::stoll(s);
    break;

  case TokenKind::Float:
    obj->type = TypeKind::Float;
    obj->vf = std::stod(s);
    break;

  case TokenKind::Char: {
    auto s16 = utils::to_u16string(s.substr(1, s.length() - 2));

    if (s16.length() != 1)
      Error(tok, "the length of character literal is must 1.")();

    obj->type = TypeKind::Char;
    obj->vc = s16[0];

    break;
  }

  case TokenKind::Boolean:
    obj->type = TypeKind::Bool;
    obj->vb = s == "true";
    break;

  default:
    todo_impl;
  }

  return obj;
}

ASTPointer Parser::Factor() {

  if (this->eat("(")) {
    auto x = this->Expr();
    this->expect(")");
    return x;
  }

  if (this->eat("[")) {
    auto x = AST::Array::New(*this->ate);

    if (!this->eat("]")) {
      do {
        x->elements.emplace_back(this->Expr());
      } while (this->eat(","));

      this->expect("]");
    }

    return x;
  }

  auto& tok = *this->cur++;

  if (tok.kind == TokenKind::Char || tok.kind == TokenKind::String) {
    auto it = tok.str.begin();
    auto end = tok.str.end();

    while (it != end) {
      if (*it == '\\') {
        switch (it[1]) {
        case 'n':
          tok.str.replace(it, it + 2, "\n");
          break;

        case 'r':
          tok.str.replace(it, it + 2, "\r");
          break;

        default:
          tok.sourceloc.position += (end - it);
          tok.sourceloc.length = 1;

          Error(tok, "invalid escape sequence")();
        }
      }

      it++;
    }
  }

  switch (tok.kind) {
  case TokenKind::Int:
  case TokenKind::Float:
  case TokenKind::Size:
  case TokenKind::Char:
  case TokenKind::Boolean:
    return AST::Value::New(tok, make_value_from_token(tok));

  case TokenKind::String:
    return AST::Value::New(
        tok, ObjNew<ObjString>(tok.str.substr(1, tok.str.length() - 2)));

  case TokenKind::Identifier: {
    static int _prs_depth = 0;

    auto x = AST::Identifier::New(tok);

    if (this->match("::", "<")) {
      alert;

      _prs_depth++;

      x->paramtok = this->cur[1];
      this->cur += 2;

      do {
        x->id_params.emplace_back(this->expectTypeName());
      } while (this->eat(","));

      if (_prs_depth >= 2 && this->match(">>")) {
        this->insert_token(">");
        this->cur++;
      }
      else {
        this->expect(">");
      }

      _prs_depth--;
    }

    return x;
  }
    /*
    define:
      fn func <T, U> (...) {
      }

    use:
      func::<int, string>(...)
    */
  }

  Error(tok, "invalid syntax")();
}

ASTPointer Parser::ScopeResol() {
  auto x = this->Factor();

  while (this->eat("::")) {
    auto& op = *(this->cur - 1);

    x = new_expr(ASTKind::ScopeResol, op, x, this->Factor());
  }

  return x;
}

ASTPointer Parser::IndexRef() {
  auto x = this->ScopeResol();

  while (this->check()) {
    auto& op = *this->cur;

    // index reference
    if (this->eat("[")) {
      x = new_expr(ASTKind::IndexRef, op, x, this->Expr());
      this->expect("]");
    }

    // member access
    else if (this->eat(".")) {
      auto rhs = this->ScopeResol();

      if (rhs->kind != ASTKind::Variable) {
        Error(op, "syntax error")();
      }

      x = new_expr(ASTKind::MemberAccess, op, x, rhs);
    }

    // call func
    else if (this->eat("(")) {
      auto call = (x = AST::CallFunc::New(x))->As<AST::CallFunc>();

      if (!this->eat(")")) {
        do {
          if (this->match(TokenKind::Identifier, ":")) {
            auto _name = this->ScopeResol();

            auto colon = this->cur++;
            auto _expr = this->Expr();

            call->args.emplace_back(new_expr(ASTKind::SpecifyArgumentName,
                                             *colon, _name, _expr));
          }
          else {
            call->args.emplace_back(this->Expr());
          }
        } while (this->eat(","));

        this->expect(")");
      }
    }
    else
      break;
  }

  return x;
}

ASTPointer Parser::Unary() {
  auto& tok = *this->cur;

  if (this->eat("-")) {
    return new_expr(ASTKind::Sub, tok,
                    AST::Value::New("0", ObjNew<ObjPrimitive>((i64)0)),
                    this->IndexRef());
  }

  this->eat("+");

  return this->IndexRef();
}

ASTPointer Parser::Mul() {
  auto x = this->Unary();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("*"))
      x = new_expr(ASTKind::Mul, op, x, this->Unary());
    else if (this->eat("/"))
      x = new_expr(ASTKind::Div, op, x, this->Unary());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Add() {
  auto x = this->Mul();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("+"))
      x = new_expr(ASTKind::Add, op, x, this->Mul());
    else if (this->eat("-"))
      x = new_expr(ASTKind::Sub, op, x, this->Mul());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Shift() {
  auto x = this->Add();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("<<"))
      x = new_expr(ASTKind::LShift, op, x, this->Add());
    else if (this->eat(">>"))
      x = new_expr(ASTKind::RShift, op, x, this->Add());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Compare() {
  auto x = this->Shift();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("=="))
      x = new_expr(ASTKind::Equal, op, x, this->Shift());
    else if (this->eat("!="))
      x = new_expr(ASTKind::Not, op,
                   new_expr(ASTKind::Equal, op, x, this->Shift()),
                   nullptr);
    else if (this->eat(">"))
      x = new_expr(ASTKind::Bigger, op, x, this->Shift());
    else if (this->eat("<"))
      x = new_expr(ASTKind::Bigger, op, this->Shift(), x);
    else if (this->eat(">="))
      x = new_expr(ASTKind::BiggerOrEqual, op, x, this->Shift());
    else if (this->eat("<="))
      x = new_expr(ASTKind::BiggerOrEqual, op, this->Shift(), x);
    else
      break;
  }

  return x;
}

ASTPointer Parser::BitCalc() {
  auto x = this->Compare();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("&"))
      x = new_expr(ASTKind::BitAND, op, x, this->Compare());
    else if (this->eat("^"))
      x = new_expr(ASTKind::BitXOR, op, x, this->Compare());
    else if (this->eat("|"))
      x = new_expr(ASTKind::BitOR, op, x, this->Compare());
    else
      break;
  }

  return x;
}

ASTPointer Parser::LogAndOr() {
  auto x = this->BitCalc();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("&&"))
      x = new_expr(ASTKind::LogAND, op, x, this->BitCalc());
    else if (this->eat("||"))
      x = new_expr(ASTKind::LogOR, op, x, this->BitCalc());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Assign() {
  auto x = this->LogAndOr();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("="))
      x = new_expr(ASTKind::Assign, op, x, this->Assign());
    else if (this->eat("*="))
      x = new_assign(ASTKind::Mul, op, x, this->Assign());
    else if (this->eat("/="))
      x = new_assign(ASTKind::Div, op, x, this->Assign());
    else if (this->eat("+="))
      x = new_assign(ASTKind::Add, op, x, this->Assign());
    else if (this->eat("-="))
      x = new_assign(ASTKind::Sub, op, x, this->Assign());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Expr() {
  return this->Assign();
}

} // namespace fire::parser