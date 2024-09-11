#pragma once

#include "AST.h"

namespace metro {

class Error {
public:
  Error(Token tok);
  Error(AST::ASTPointer ast);

  Error& set_message(std::string msg);

  Error& emit(bool as_warn = false);

  Error& emit_as_hint();

  [[noreturn]]
  void stop();

private:
  Token loc_token;
  AST::ASTPointer loc_ast = nullptr;

  std::string msg;
};

} // namespace metro