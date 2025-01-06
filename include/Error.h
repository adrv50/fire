#pragma once

#include <concepts>
#include <functional>

#include "typedef.h"

struct Token;
struct Node;

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

  //
  // runtime error
  //
  //  usage:
  //    "index out of range"
  //    ...
  RunTimeError,
};

enum class ErrorType {
  Err,
  Warn,
  Note,
  RunTime,
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

  Token* get_token() const {
    return this->tok;
  }

  Node* get_node() const {
    return this->node;
  }

  template <ErrorKind K, typename... Args>
  Error& set_msg(Args&&... args);

  Error& set_message(string const& msg);

  Error& set_kind(ErrorKind kind) {
    this->kind = kind;
    return *this;
  }

  Error& append_msg(string const& msg) {
    this->msg += msg;
    return *this;
  }

  Error& append_msg_if(std::function<void(string&)> const& msg_fn) {
    msg_fn(this->msg);

    return *this;
  }

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
