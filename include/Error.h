#pragma once

#include "AST.h"
#include "Utils.h"

namespace fire {

class Error {
public:
  enum class ErrorLevel {
    Error,
    Warning,
    Note,
  };

  Error(Token tok, std::string msg = "");
  Error(ASTPointer ast, std::string msg = "");

  template <class... Args>
  Error& format(std::string const& fmt, Args&&... args) {
    this->msg = utils::Format(fmt, std::forward<Args>(args)...);
    return *this;
  }

  Error& emit(ErrorLevel lv = ErrorLevel::Error);

  static int GetEmittedCount();

  [[noreturn]] void operator()();

  [[noreturn]] void stop();

  [[noreturn]] static void fatal_error(std::string const& msg);

private:
  Token loc_token;
  ASTPointer loc_ast = nullptr;

  std::string msg;
};

} // namespace fire