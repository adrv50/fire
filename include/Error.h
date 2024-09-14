#pragma once

#include "AST.h"
#include "Utils.h"

namespace metro {

class Error {
public:
  Error(Token tok, std::string msg = "");
  Error(ASTPointer ast, std::string msg = "");

  template <class... Args>
  Error& format(std::string const& fmt, Args&&... args) {
    this->msg = utils::Format(fmt, std::forward<Args>(args)...);
    return *this;
  }

  Error& emit(bool as_warn = false);

  [[noreturn]]
  void operator()();

  [[noreturn]]
  void stop();

  [[noreturn]]
  static void fatal_error(std::string const& msg);

private:
  Token loc_token;
  ASTPointer loc_ast = nullptr;

  std::string msg;
};

} // namespace metro