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

} // namespace fire::parser