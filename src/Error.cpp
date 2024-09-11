#include <iostream>
#include "Error.h"
#include "Utils.h"
#include "alert.h"

namespace metro {

Error::Error(Token tok) : loc_token(tok) {
}

Error::Error(AST::ASTPointer ast) : loc_ast(ast) {
}

Error& Error::set_message(std::string msg) {
  this->msg = std::move(msg);
  return *this;
}

Error& Error::emit(bool as_warn) {

  if (this->loc_ast) {
  }
  else {
    auto& loc = this->loc_token.sourceloc;
    auto const& src = *loc.ref;

    std::cout << COL_BOLD COL_WHITE << src.path << ":"
              << loc.line.index + 1 << ":" << loc.pos_in_line << ": "
              << (as_warn ? COL_MAGENTA "warning: "
                          : COL_RED "error: ")
              << this->msg << std::endl
              << COL_CYAN "      |\n"
              << utils::Format(COL_WHITE "% 5d " COL_CYAN
                                         "| " COL_WHITE,
                               loc.line.index + 1)
              << loc.GetLine() << std::endl
              << COL_CYAN "      | "
              << std::string(loc.pos_in_line, ' ') << "^" << std::endl
              << COL_CYAN "      |\n";
  }

  return *this;
}

// Error& Error::emit_as_hint() {
// }

[[noreturn]]
void Error::stop() {
  std::exit(1);
}

} // namespace metro