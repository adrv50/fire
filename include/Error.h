#pragma once

#include "AST.h"

namespace metro {

class Error {
public:
  Error(Token tok, std::string msg);
  Error(ASTPointer ast, std::string msg);

  [[noreturn]]
  void operator()() {
    this->emit().stop();
  }

  Error& emit(bool as_warn = false);

  [[noreturn]]
  void stop();

private:
  Token loc_token;
  ASTPointer loc_ast = nullptr;

  std::string msg;
};

} // namespace metro