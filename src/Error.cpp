#include <iostream>

#include "alert.h"
#include "Utils.h"

#include "Token/Token.h"
#include "Node/Node.h"

#include "Driver/Error.h"

namespace fire {

Error::Error(Token* tok, string const& msg, ErrorType type)
    : type(type),
      tok(tok),
      node(nullptr),
      msg(msg) {
}

Error::Error(Node* node, string const& msg, ErrorType type)
    : type(type),
      tok(nullptr),
      node(node),
      msg(msg) {
}

Error& Error::set_message(string const& msg) {
  this->msg = msg;
  return *this;
}

Error const& Error::emit() const {
  using std::cout;
  using std::endl;

  switch (this->type) {
    case ErrorType::Err:
      cout << COL_BOLD COL_RED << "error: ";
      break;

    case ErrorType::Warn:
      cout << COL_BOLD COL_MAGENTA << "warning: ";
      break;

    case ErrorType::Note:
      cout << COL_BOLD COL_GREEN << "note: ";
      break;

    case ErrorType::RunTime:
      cout << COL_BOLD COL_RED << "runtime error: ";
      break;
  }

  cout << COL_WHITE << this->msg << endl << COL_DEFAULT;

  auto tok = this->tok;

  if (this->tok || this->node) {
    if (!tok) {
      if (this->node->first_tok)
        tok = this->node->first_tok;
      else
        tok = this->node->tok;
    }

    assert(tok != nullptr);

    auto& ref = tok->ref;

    if (auto ss = ref->get_ss()) {
      string lineview = string(ref->get_line_view());

      if (!this->errpos_insert_text.empty())
        lineview.insert((size_t)((i64)(ref->pos_in_line - 1) + (i64)(this->insert_dist)),
                        COL_GREEN + this->errpos_insert_text + COL_WHITE);

      cout << COL_YELLOW "     ---> " << COL_CYAN << ss->get_path() << ":"
           << ref->line_num << ":" << ref->pos_in_line << endl
           << COL_YELLOW << "     |" << endl
           << utils::format("% 4zu | ", ref->line_num) << COL_WHITE << lineview
           << COL_YELLOW "     |" << COL_RED << string(ref->pos_in_line, ' ') << "^ "
           << COL_GREEN << this->cursor_text << endl
           << endl
           << COL_DEFAULT;
    }
  }

  for (auto&& note : this->notes)
    note.emit();

  if (this->type != ErrorType::Note)
    cout << endl;

  return *this;
}

void Error::stop(int code) {
  std::exit(code);
}

// --------------------------------------------
//  Instantiations of set_msg()

#define INST(_K, _Args...)                                                               \
  template <>                                                                            \
  Error& Error::set_msg<_K>(_Args)

using Ek = ErrorKind;

//
// TypeMismatch

INST(Ek::TypeMismatch) {
  this->set_message("type mismatch");
  return *this;
}

//
// UnexpectedType
//
INST(Ek::UnexpectedType, string const& expected, string const& found) {
  this->set_message("expected type '" + expected + "', but found '" + found + "'");
  return *this;
}

//
// UnexpectedToken
//
INST(Ek::UnexpectedToken, string const& token) {
  this->set_message("unexpected token '" + token + "'");
  return *this;
}

//
// InvalidToken

INST(Ek::InvalidToken, string const& token) {
  this->set_message("invalid token '" + token + "'");
  return *this;
}

//
// InvalidSyntax
//
INST(Ek::InvalidSyntax) {
  this->set_message("invalid syntax");
  return *this;
}

//
// UndefinedName
//
INST(Ek::UndefinedName, string const& name) {
  this->set_message("the name '" + name + "' is not defined");
  return *this;
}

//
// NotSubscriptable
//
INST(Ek::NotSubscriptable, string const& type) {
  this->set_message("'" + type + "' type is not subscriptable");
  return *this;
}

//
// NotIterable
//
INST(Ek::NotIterable, string const& type) {
  this->set_message("'" + type + "' type is not iterable");
  return *this;
}

//
// InvalidOperatorForType
//
INST(Ek::InvalidOperatorForType, TypeInfo const& type, string const& op) {
  this->set_message("'" + type.to_string() + "' type does not support operator '" + op +
                    "'");
  return *this;
}

//
// NotAllowedInThisContext
//
INST(Ek::NotAllowedInThisContext, string const& keyword) {
  this->set_message("cannot use '" + keyword + "' in this context");

  return *this;
}

// ========================================
//

} // namespace fire