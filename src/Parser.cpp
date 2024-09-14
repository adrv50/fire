#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace metro::parser {

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

Parser::Parser(TokenVector tokens)
    : tokens(std::move(tokens)),
      cur(this->tokens.begin()),
      end(this->tokens.end()) {
}

ASTPointer Parser::Factor() {
  auto& tok = *this->cur++;

  switch (tok.kind) {
  case TokenKind::Int:
  case TokenKind::Float:
  case TokenKind::Size:
  case TokenKind::Char:
  case TokenKind::Boolean:
    return AST::Value::New(tok, make_value_from_token(tok));

  case TokenKind::String:
    return AST::Value::New(tok, ObjNew<ObjString>(tok.str.substr(
                                    1, tok.str.length() - 2)));

  case TokenKind::Identifier:
    return AST::Variable::New(tok);
  }

  Error(tok, "invalid syntax")();
}

ASTPointer Parser::IndexRef() {
  auto x = this->Factor();

  while (this->check()) {
    auto& op = *this->cur;

    // index reference
    if (this->eat("[")) {
      x = new_expr(ASTKind::IndexRef, op, x, this->Factor());
      this->expect("]");
    }

    // member access
    else if (this->eat(".")) {
      auto rhs = this->Factor();

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
            auto _name = this->Factor();

            auto colon = this->cur++;
            auto _expr = this->Expr();

            call->args.emplace_back(new_expr(
                ASTKind::SpecifyArgumentName, *colon, _name, _expr));
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
    return new_expr(
        ASTKind::Sub, tok,
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

ASTPointer Parser::Stmt() {

  auto tok = *this->cur;

  if (this->eat("{")) {
    auto ast = AST::Block::New(tok);

    ast->token = tok;

    if (this->eat("}")) {
      ast->endtok = *this->ate;
      return ast;
    }

    while (this->check()) {
      ast->list.emplace_back(this->Stmt());

      if (this->eat("}")) {
        ast->endtok = *this->ate;
        return ast;
      }
    }

    Error(tok, "not terminated block")();
  }

  if (this->eat("if")) {
    auto cond = this->Expr();

    this->expect("{", true);
    auto if_true = this->Stmt();

    ASTPointer if_false = nullptr;

    if (this->eat("else")) {
      if (!this->eat("if"))
        this->expect("{", true);

      if_false = this->Stmt();
    }

    return AST::Statement::NewIf(tok, cond, if_true, if_false);
  }

  if (this->eat("let")) {
    auto ast = AST::VarDef::New(tok, *this->expectIdentifier());

    if (this->eat(":"))
      ast->type = this->expectTypeName();

    if (this->eat("="))
      ast->init = this->Expr();

    this->expect(";");
    return ast;
  }

  auto ast = this->Expr();

  this->expect(";");

  return ast;
}

ASTPointer Parser::Top() {

  auto tok = *this->cur;
  auto iter = this->cur;

  if (this->eat("enum")) {
    auto ast = AST::Enum::New(tok, *this->expectIdentifier());

    this->expect("{");

    if (this->eat("}")) {
      Error(*this->ate, "empty enum is not valid").emit();

      return ast;
    }

    do {
      auto& etor = ast->enumerators.emplace_back(
          AST::Enumerator::New(*this->expectIdentifier()));

      if (this->eat("(")) {
        etor->value_type = this->expectTypeName();
        this->expect(")");
      }
    } while (this->eat(","));

    this->expect("}");

    return ast;
  }

  if (this->eat("class")) {
    auto ast = AST::Class::New(tok, *this->expectIdentifier());

    this->expect("{");

    bool closed = false;

    // member variables
    while (this->cur->str == "let") {
      ast->mb_variables.emplace_back(
          std::static_pointer_cast<AST::VarDef>(this->Stmt()));
    }

    // constructor
    if (this->eat(ast->GetName())) {

      auto ctor = AST::Function::New(ast->name, ast->name);

      this->expect("(");

      this->expect("self");
      ctor->add_arg(*this->ate);

      while (this->eat(",")) {
        ctor->add_arg(*this->expectIdentifier());
      }

      this->expect(")");

      this->expect("{", true);
      ctor->block = ASTCast<AST::Block>(this->Stmt());

      ast->constructor = ctor;
    }

    // member functions
    while (this->check() && !(closed = this->eat("}"))) {
      if (!this->match("fn", TokenKind::Identifier))
        Error(*this->cur, "expected definition of member function")();

      ast->mb_functions.emplace_back(
          std::static_pointer_cast<AST::Function>(this->Top()));
    }

    if (!closed)
      Error(iter[2], "not terminated block")();

    return ast;
  }

  if (this->eat("fn")) {
    auto func = AST::Function::New(tok, *this->expectIdentifier());

    this->expect("(");

    if (!this->eat(")")) {
      do {
        func->add_arg(*this->expectIdentifier());
      } while (this->eat(","));

      this->expect(")");
    }

    if (this->eat("->")) {
      func->return_type = this->expectTypeName();
    }

    if (this->cur->str != "{")
      todo_impl;

    func->block = std::static_pointer_cast<AST::Block>(this->Stmt());

    return func;
  }

  //
  // import <name>
  //   --> convert to " let name = import("name"); "
  //
  // replace to variable declaration,
  //  and assignment result of call "import" func.
  //
  if (this->match("import", TokenKind::Identifier, ";") &&
      this->eat("import")) {
    std::string name = this->expectIdentifier()->str;

    while (this->eat("/")) {
      name += "/" + this->expectIdentifier()->str;
    }

    this->expect(";");

    auto ast = AST::VarDef::New(tok, iter[1]);

    auto call = AST::CallFunc::New(AST::Variable::New(tok));

    Token mod_name_token = iter[1];

    mod_name_token.kind = TokenKind::String;
    mod_name_token.str = name;
    mod_name_token.sourceloc.length = name.length();

    auto module_name =
        AST::Value::New(mod_name_token, ObjNew<ObjString>(name));

    ast->init = call;

    return ast;
  }

  return this->Stmt();
}

ASTPtr<AST::Program> Parser::Parse() {
  auto ret = AST::Program::New();

  while (this->check())
    ret->list.emplace_back(this->Top());

  return ret;
}

bool Parser::check() const {
  return this->cur < this->end;
}

bool Parser::eat(std::string_view str) {
  if (this->check() && this->cur->str == str) {
    this->ate = this->cur++;
    return true;
  }

  return false;
}

void Parser::expect(std::string_view str, bool no_next) {
  if (!this->eat(str)) {
    Error(*this->cur, "expected '" + std::string(str) +
                          "' buf found '" +
                          std::string(this->cur->str) + "'")();
  }

  if (no_next)
    this->cur--;
}

TokenIterator Parser::expectIdentifier() {
  if (this->cur->kind != TokenKind::Identifier) {
    Error(*this->cur, "expected identifier but found '" +
                          std::string(this->cur->str) + "'")();
  }

  return this->cur++;
}

ASTPtr<AST::TypeName> Parser::expectTypeName() {
  auto ast = AST::TypeName::New(*this->expectIdentifier());

  if (this->eat("<")) {
    do {
      ast->type_params.emplace_back(this->expectTypeName());
    } while (this->eat(","));

    if (this->cur->str == ">>") {
      this->cur->str = ">";
      this->cur = this->insert_token(">") + 1;
    }
    else {
      this->expect(">");
    }
  }

  return ast;
}

} // namespace metro::parser