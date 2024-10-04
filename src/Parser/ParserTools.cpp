#include <iostream>

#include "alert.h"
#include "Parser.h"
#include "Error.h"
#include "Utils.h"

namespace fire::parser {

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
    throw Error(*this->cur, "expected '" + std::string(str) + "' buf found '" +
                                std::string(this->cur->str) + "'");
  }

  if (no_next)
    this->cur--;
}

bool Parser::eat_typeparam_bracket_open() {
  if (this->eat("<")) {
    _typeparam_bracket_depth++;
    return true;
  }

  return false;
}

bool Parser::eat_typeparam_bracket_close() {
  if (_typeparam_bracket_depth >= 1 && this->match(">>")) {
    this->cur->str = ">";
    this->cur = this->insert_token(">");
  }

  if (this->eat(">")) {
    _typeparam_bracket_depth--;
    return true;
  }

  return false;
}

void Parser::expect_typeparam_bracket_close() {
  if (!this->eat_typeparam_bracket_close()) {
    throw Error(*this->cur, "expected '>'");
  }
}

TokenIterator Parser::expectIdentifier() {
  if (this->cur->kind != TokenKind::Identifier) {
    throw Error(*(this->cur - 1), "expected identifier after this token");
  }

  return this->cur++;
}

ASTPtr<AST::TypeName> Parser::expectTypeName() {
  auto ast = AST::TypeName::New(*this->expectIdentifier());

  if (this->eat_typeparam_bracket_open()) {
    do {
      ast->type_params.emplace_back(this->expectTypeName());
    } while (this->eat(","));

    this->expect_typeparam_bracket_close();
  }

  return ast;
}

ASTPtr<AST::Signature> Parser::parse_signature() {
  auto tok = *this->cur;

  this->expect("(");

  ASTVec<AST::TypeName> args;

  if (!this->eat(")")) {
    do {
      args.emplace_back(this->expectTypeName());
    } while (this->eat(","));

    this->expect(")");
  }

  ASTPtr<AST::TypeName> restype = nullptr;

  if (this->eat("->")) {
    restype = this->expectTypeName();
  }

  return AST::Signature::New(tok, std::move(args), restype);
}

} // namespace fire::parser