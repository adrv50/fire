#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace fire::parser {

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

  if (this->eat("return")) {
    auto ast =
        AST::Statement::New(ASTKind::Return, tok, this->Expr());

    this->expect(";");
    return ast;
  }

  if (this->eat("break")) {
    auto ast = AST::Statement::New(ASTKind::Break, tok);

    this->expect(";");
    return ast;
  }

  if (this->eat("continue")) {
    auto ast = AST::Statement::New(ASTKind::Continue, tok);

    this->expect(";");
    return ast;
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
      ast->enumerators.emplace_back(*this->expectIdentifier());
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
          ASTCast<AST::VarDef>(this->Stmt()));
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
          ASTCast<AST::Function>(this->Top()));
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

    func->block = ASTCast<AST::Block>(this->Stmt());

    return func;
  }

  //
  // import <name>
  //   --> convert to " let name = import("name"); "
  //
  // replace to variable declaration,
  //  and assignment result of call "import" func.
  //
  if (this->match("import", TokenKind::Identifier) ||
      this->match("import", ".")) {

    // import a;
    // import ./b;
    // import ../c;

    this->cur++;
    std::string name;

    if (this->eat(".")) {
      name += ".";

      if (this->eat("."))
        name += ".";

      this->expect("/");
      name += "/";
    }

    name = this->expectIdentifier()->str;

    while (this->eat("/")) {
      name += "/" + this->expectIdentifier()->str;
    }

    name = name + ".fire";

    this->expect(";");

    auto ast = AST::VarDef::New(tok, utils::get_base_name(name));

    auto call = AST::CallFunc::New(AST::Variable::New("@import"));

    Token mod_name_token = iter[1]; // for argument of @import

    mod_name_token.kind = TokenKind::String;
    mod_name_token.str = name;
    mod_name_token.sourceloc.length = name.length();

    call->args.emplace_back(
        AST::Value::New(mod_name_token, ObjNew<ObjString>(name)));

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

Parser::Parser(TokenVector tokens)
    : tokens(std::move(tokens)),
      cur(this->tokens.begin()),
      end(this->tokens.end()) {
}

} // namespace fire::parser