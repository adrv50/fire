#pragma once

#include "AST.h"
#include "Utils.h"

namespace fire {

class Error {
public:
  enum ErrorKind {
    ER_Error,
    ER_Warning,
    ER_Note,
  };

  Error(ErrorKind k, Token tok, std::string msg = "");
  Error(ErrorKind k, ASTPointer ast, std::string msg = "");

  Error(Token tok, std::string msg = "");
  Error(ASTPointer ast, std::string msg = "");

  template <class... Args>
  Error& format(std::string const& fmt, Args&&... args) {
    this->msg = utils::Format(fmt, std::forward<Args>(args)...);
    return *this;
  }

  Error const& emit() const;

  Error& InLocation(string const& loc) {
    this->location.emplace_back(loc);
    return *this;
  }

  Error& AddNote(string const& note) {
    this->notes.emplace_back(note);
    return *this;
  }

  Error& AddChain(Error e) {
    this->chained.emplace_back(std::move(e));
    return *this;
  }

  static int GetEmittedCount();

  [[noreturn]] void operator()();

  [[noreturn]] void stop();

  [[noreturn]] static void fatal_error(std::string const& msg);

private:
  ErrorKind kind;

  Token loc_token;
  ASTPointer loc_ast = nullptr;

  std::string msg;

  StringVector location;
  StringVector notes;

  vector<Error> chained;
};

} // namespace fire