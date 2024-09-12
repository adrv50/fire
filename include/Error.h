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