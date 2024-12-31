#pragma once

#include <concepts>

#include "Utils.h"
#include "Node.h"

enum class ErrorKind {
  //
  // default
  None,

  //
  // type mismatch
  TypeMismatch,

  //
  // unexpected type
  //
  //  usage:
  //    "expected type 'int', but found 'string'"
  UnexpectedType,

  //
  // unexpected token
  UnexpectedToken,

  //
  // invalid token (not supported)
  InvalidToken,

  //
  // invalid syntax
  InvalidSyntax,

  //
  // undefined name
  UndefinedName,

  //
  // operator errors
  NotSubscriptable,
  NotIterable,
  InvalidOperatorForType,

  //
  // not allowed in this context
  //
  //  usage:
  //    "cannot use 'return' outside of function"
  //    "cannot use 'break' outside of loop"
  //    ...
  NotAllowedInThisContext,
};

enum class ErrorType {
  Err,
  Warn,
  Note,
};

class Error {

  ErrorKind kind = ErrorKind::None;
  ErrorType type;

  Token* tok;
  Node* node;

  string msg;

  Vec<Error> notes;

public:
  Error(Token* tok, string const& msg = "", ErrorType type = ErrorType::Err);
  Error(Node* node, string const& msg = "", ErrorType type = ErrorType::Err);

  Error(string const& msg, ErrorType type = ErrorType::Err)
      : Error((Token*)nullptr, msg, type) {
  }

  template <ErrorKind K, typename... Args>
  Error& set_msg(Args&&... args);
  // => instantiated in Error.cpp

  Error& set_message(string const& msg);

  template <typename... Args>
  requires std::constructible_from<Error, Args...>
  Error& add_note(Args&&... args) {
    this->notes.emplace_back(std::forward<Args>(args)...).type = ErrorType::Note;

    return *this;
  }

  Error const& emit() const;

  [[noreturn]]
  void stop(int code = 1);

  //
  // crash:
  //   emit and exit.
  [[noreturn]]
  void crash() {
    throw *this;
  }
};
