#pragma once

namespace fire::AST {

struct Named : Base {
  Token name;

  string const& GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind k, Token tok, Token name)
      : Base(k, tok),
        name(name) {
    this->_attr.IsNamed = true;
  }

  Named(ASTKind k, Token t)
      : Named(k, t, t) {
  }
};

} // namespace fire::AST