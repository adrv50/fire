#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace metro::parser {

Parser::Parser(TokenVector tokens)
    : tokens(std::move(tokens)), cur(this->tokens.begin()),
      end(this->tokens.end()) {
}

ASTPointer Parser::Factor() {
  auto& tok = *this->cur++;

  switch (tok.kind) {
  case TokenKind::Int:
  case TokenKind::Float:
  case TokenKind::Size:
  case TokenKind::Char:
  case TokenKind::Boolean: {
    static auto f =
        [](TokenKind k,
           std::string const& s) -> std::shared_ptr<ObjPrimitive> {
      switch (k) {
      case TokenKind::Int:
        return ObjNew<ObjPrimitive>((i64)std::stoll(s));

      case TokenKind::Float:
        return ObjNew<ObjPrimitive>((double)std::stod(s));

      case TokenKind::Char:
        return ObjNew<ObjPrimitive>(utils::to_u16string(s)[1]);

      case TokenKind::Boolean:
        return ObjNew<ObjPrimitive>(s == "true");
      }

      return nullptr;
    };

    return ASTNew<AST::Value>(tok, f(tok.kind, std::string(tok.str)));
  }

  case TokenKind::String:
    return ASTNew<AST::Value>(
        tok, ObjNew<ObjString>(std::string(
                 tok.str.substr(1, tok.str.length() - 2))));

  case TokenKind::Identifier: {
    todo_impl;
  }
  }

  Error(tok).set_message("invalid syntax").emit().stop();
}

ASTPointer Parser::ScopeResol() {
  auto x = this->Factor();

  while (this->check() && this->eat("::")) {
    auto& op = *this->ate;

    x = ASTNew<AST::Expr>(ASTKind::ScopeResolution, op, x,
                          this->Factor());
  }

  return x;
}

ASTPointer Parser::IndexRef() {
  auto x = this->ScopeResol();

  while (this->check()) {
    auto& op = *this->cur;

    if (this->eat("[")) {
      x = ASTNew<AST::Expr>(ASTKind::IndexRef, op, x,
                            this->ScopeResol());
      this->expect("]");
    }
    else if (this->eat(".")) {
      x = ASTNew<AST::Expr>(ASTKind::MemberAccess, op, x,
                            this->ScopeResol());
    }
    else if (this->eat("(")) {
      x = ASTNew<AST::CallFunc>(x);

      if (!this->eat(")")) {
        do {
          x->As<AST::CallFunc>()->args.emplace_back(this->Expr());
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
    return ASTNew<AST::Expr>(
        ASTKind::Sub, tok,
        ASTNew<AST::Value>("0", ObjNew<ObjPrimitive>((i64)0)),
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
      x = ASTNew<AST::Expr>(ASTKind::Mul, op, x, this->Unary());
    else if (this->eat("/"))
      x = ASTNew<AST::Expr>(ASTKind::Div, op, x, this->Unary());
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
      x = ASTNew<AST::Expr>(ASTKind::Add, op, x, this->Mul());
    else if (this->eat("-"))
      x = ASTNew<AST::Expr>(ASTKind::Sub, op, x, this->Mul());
    else
      break;
  }

  return x;
}

ASTPointer Parser::Expr() {
  return this->Add();
}

ASTPointer Parser::Stmt() {

  auto tok = *this->cur;
  auto iter = this->cur;

  if (this->eat("{")) {
    auto ast = ASTNew<AST::Block>();

    ast->token = tok;

    if (this->eat("}")) {
      ast->endtok = *this->ate;
      return ast;
    }

    cur_scope_depth++;

    while (this->check()) {
      ast->list.emplace_back(this->Stmt());

      if (this->eat("}")) {
        ast->endtok = *this->ate;
        goto block_closed;
      }
    }

    Error(tok).set_message("not terminated block").emit().stop();

  block_closed:;
    cur_scope_depth--;

    return ast;
  }

  if (this->eat("if")) {
    auto data = AST::Statement::If{};

    data.cond = this->Expr();

    this->expect("{", true);
    data.if_true = this->Stmt();

    if (this->eat("else")) {
      if (!this->eat("if"))
        this->expect("{", true);

      data.if_false = this->Stmt();
    }

    return ASTNew<AST::Statement>(ASTKind::If, tok, data);
  }

  if (this->eat("let")) {
    auto ast = ASTNew<AST::VarDef>(tok, *this->expectIdentifier());

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
    auto ast = ASTNew<AST::Enum>(tok, *this->expectIdentifier());

    this->expect("{");

    if (this->eat("}")) {
      Error(*this->ate).set_message("empty enum is not valid").emit();
      return ast;
    }

    cur_scope_depth++;

    do {
      auto& etor = ast->enumerators.emplace_back(
          std::make_shared<AST::Enumerator>(
              *this->expectIdentifier()));

      if (this->eat("(")) {
        etor->value_type = this->expectTypeName();
        this->expect(")");
      }
    } while (this->eat(","));

    this->expect("}");
    cur_scope_depth--;

    return ast;
  }

  if (this->eat("class")) {
    auto ast = ASTNew<AST::Class>(tok, *this->expectIdentifier());

    this->expect("{");
    cur_scope_depth++;

    bool closed = false;

    while (this->cur->str == "let") {
      ast->mb_variables.emplace_back(
          std::static_pointer_cast<AST::VarDef>(this->Stmt()));
    }

    while (this->check() && !(closed = this->eat("}"))) {
      if (!this->match("fn"))
        Error(*this->cur)
            .set_message("expected definition of member function")
            .emit()
            .stop();

      ast->mb_functions.emplace_back(
          std::static_pointer_cast<AST::Function>(this->Top()));
    }

    cur_scope_depth--;

    if (!closed)
      Error(iter[2])
          .set_message("not terminated block")
          .emit()
          .stop();

    return ast;
  }

  if (this->eat("fn")) {
    auto func = ASTNew<AST::Function>(tok, *this->expectIdentifier());

    cur_scope_depth++;

    this->expect("(");

    if (!this->eat(")")) {
      do {
        auto name = this->expectIdentifier();

        this->expect(":");
        auto type = this->expectTypeName();

        func->args.emplace_back(
            std::make_shared<AST::FuncArgument>(*name, type));

      } while (this->eat(","));

      this->expect(")");
    }

    if (this->eat("->")) {
      func->return_type = this->expectTypeName();
    }

    if (this->cur->str != "{")
      todo_impl;

    func->block = std::static_pointer_cast<AST::Block>(this->Stmt());

    cur_scope_depth--;

    return func;
  }

  if (this->eat("namespace")) {

    auto ast = ASTNew<AST::Namespace>(tok, *this->expectIdentifier());

    this->expect("{");
    cur_scope_depth++;

    bool closed = false;

    while (this->check() && !(closed = this->eat("}"))) {
      ast->list.emplace_back(this->Top());
    }

    if (!closed)
      Error(ast->token)
          .set_message("not terminated block")
          .emit()
          .stop();

    cur_scope_depth--;

    return ast;
  }

  Error(*this->cur)
      .set_message("expected enum or class or function or namespace")
      .emit()
      .stop();
}

AST::Program Parser::Parse() {
  AST::Program ret;

  this->MakeIdentifierTags();

  alert;
  for (auto&& [name, tag] : this->id_tag_map) {
    std::cout << utils::Format("%.*s\t: depth=% 2d, type=%d",
                               name.length(), name.data(),
                               tag.scope_depth, tag.type)
              << std::endl;
  }

  while (this->check())
    ret.list.emplace_back(this->Top());

  return ret;
}

void Parser::MakeIdentifierTags() {
  int scope_depth = 0;

  while (this->check()) {
    if (this->eat("{")) {
      scope_depth++;
    }

    else if (this->eat("}")) {
      scope_depth--;
    }

    else if (this->match("let", TokenKind::Identifier)) {
      this->add_id_tag(this->cur[1].str,
                       IdentifierTag(IdentifierTag::Variable,
                                     this->cur + 1, scope_depth));

      this->cur += 2;
    }

    else if (this->match("namespace", TokenKind::Identifier)) {
      this->add_id_tag(this->cur[1].str,
                       IdentifierTag(IdentifierTag::Namespace,
                                     this->cur + 1, scope_depth));

      this->cur += 2;
    }

    else if (this->match("enum", TokenKind::Identifier)) {
      this->add_id_tag(this->cur[1].str,
                       IdentifierTag(IdentifierTag::Enum,
                                     this->cur + 1, scope_depth));

      this->cur += 2;
    }

    else if (this->match("class", TokenKind::Identifier)) {
      this->add_id_tag(this->cur[1].str,
                       IdentifierTag(IdentifierTag::Class,
                                     this->cur + 1, scope_depth));

      this->cur += 2;
    }

    else if (this->match("fn", TokenKind::Identifier)) {
      this->add_id_tag(this->cur[1].str,
                       IdentifierTag(IdentifierTag::Function,
                                     this->cur + 1, scope_depth));

      this->cur += 2;
    }

    else {
      this->cur++;
    }
  }

  this->cur = this->tokens.begin();
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
    Error(*this->cur)
        .set_message("expected '" + std::string(str) +
                     "' buf found '" + std::string(this->cur->str) +
                     "'")
        .emit()
        .stop();
  }

  if (no_next)
    this->cur--;
}

TokenIterator Parser::expectIdentifier() {
  if (this->cur->kind != TokenKind::Identifier) {
    Error(*this->cur)
        .set_message("expected identifier but found '" +
                     std::string(this->cur->str) + "'")
        .emit()
        .stop();
  }

  return this->cur++;
}

ASTPtr<AST::TypeName> Parser::expectTypeName() {
  auto ast = ASTNew<AST::TypeName>(*this->expectIdentifier());

  if (!this->is_type_name(ast->token.str)) {
    Error(ast->token).set_message("expected type name").emit().stop();
  }

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