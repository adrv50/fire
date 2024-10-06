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
  case TokenKind::Int: {
    obj->type = TypeKind::Int;
    obj->vi = atoll(s.data());
    break;
  }

  case TokenKind::Float: {
    obj->type = TypeKind::Float;
    obj->vf = atof(s.data());
    break;
  }

  case TokenKind::Char: {
    auto s16 = utils::to_u16string(string(s.substr(1, s.length() - 2)));

    if (s16.length() != 1)
      throw Error(tok, "the length of character literal is must 1.");

    obj->type = TypeKind::Char;
    obj->vc = s16[0];

    break;
  }

  case TokenKind::Boolean: {

    obj->type = TypeKind::Bool;
    obj->vb = s == "true";
    break;
  }

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
        size_t index = tok.sourceloc.position + (it - tok.str.begin());

        switch (it[1]) {
        case 'n':
          tok.get_src_data().replace(index, index + 2, "\n");
          break;

        case 'r':
          tok.get_src_data().replace(index, index + 2, "\r");
          break;

        default:
          tok.sourceloc.position += (end - it);
          tok.sourceloc.length = 1;

          throw Error(tok, "invalid escape sequence");
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

  case TokenKind::String: {
    auto xx = AST::Value::New(
        tok, ObjNew<ObjString>(string(tok.str.substr(1, tok.str.length() - 2))));

    return xx;
  }

  case TokenKind::Identifier: {
    auto x = AST::Identifier::New(tok);

    if (this->eat("@")) {

      x->paramtok = *this->cur;

      this->expect("<");

      do {
        auto& templ_arg = x->id_params.emplace_back(this->ScopeResol());

        if (!templ_arg->is_ident_or_scoperesol()) {
          throw Error(templ_arg, "invalid syntax");
        }
      } while (this->eat(","));

      this->expect_typeparam_bracket_close();
    }

    return x;
  }
  }

  throw Error(tok, "invalid syntax");
}

ASTPointer Parser::ScopeResol() {
  auto x = this->Factor();

  //
  // scope-resolution
  //
  if (this->match("::")) {
    if (x->kind != ASTKind::Identifier)
      throw Error(*this->cur, "invalid syntax");

    auto sr = AST::ScopeResol::New(ASTCast<AST::Identifier>(x));

    while (this->eat("::")) {
      auto op = *this->ate;

      if (sr->idlist.emplace_back(ASTCast<AST::Identifier>(this->Factor()))->kind !=
          ASTKind::Identifier)
        throw Error(op, "invalid syntax");
    }

    x = sr;
  }

  //
  // overload-reslotion-guide
  //
  if (this->eat("of")) {
    if (!x->is_ident_or_scoperesol())
      throw Error(*this->ate, "invalid syntax");

    auto op = *this->ate;

    return new_expr(ASTKind::OverloadResolutionGuide, op, x, this->expect_signature());
  }

  return x;
}

ASTPointer Parser::Lambda() {

  if (this->eat("lambda")) {
    auto tok = *this->ate;

    this->expect("(");

    ASTVec<AST::Argument> args;
    bool is_var_arg = false;

    if (!this->eat(")")) {
      do {
        auto name = *this->expectIdentifier();

        this->expect(":");
        auto type = this->expectTypeName();

        args.emplace_back(AST::Argument::New(name, type));
      } while (this->eat(","));

      this->expect(")");
    }

    ASTPtr<AST::TypeName> rettype = nullptr;

    if (this->eat("->")) {
      rettype = this->expectTypeName();
    }

    this->expect("{");
    auto block = ASTCast<AST::Block>(this->Stmt());

    auto ast = AST::Function::New(tok, tok, std::move(args), is_var_arg, rettype, block);

    ast->kind = ASTKind::LambdaFunc;

    return ast;
  }

  return this->ScopeResol();
}

ASTPointer Parser::IndexRef() {
  auto x = this->Lambda();

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

      if (rhs->kind != ASTKind::Identifier)
        throw Error(op, "syntax error");

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

            call->args.emplace_back(
                new_expr(ASTKind::SpecifyArgumentName, *colon, _name, _expr));
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
    return new_expr(ASTKind::Sub, tok, AST::Value::New("0", ObjNew<ObjPrimitive>((i64)0)),
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

  int count = 0;
  TokenIterator _tok = this->cur;

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("=="))
      x = new_expr(ASTKind::Equal, op, x, this->Shift());
    else if (this->eat("!="))
      x = new_expr(ASTKind::Not, op, new_expr(ASTKind::Equal, op, x, this->Shift()),
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

    if (++count >= 2) {
      if (_tok->str == "<" && (_tok - 1)->kind == TokenKind::Identifier) {
        throw Error(op, "compare operator cannot be chained")
            .AddNote("if you want to give template argument, write as '@<...>'");
      }
    }
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