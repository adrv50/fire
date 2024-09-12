#include <iostream>
#include "Error.h"
#include "Utils.h"
#include "alert.h"

namespace metro {

Error::Error(Token tok, std::string msg)
    : loc_token(tok), msg(std::move(msg)) {
}

Error::Error(ASTPointer ast, std::string msg)
    : loc_ast(ast), msg(std::move(msg)) {
}

Error& Error::emit(bool as_warn) {
  auto& loc = this->loc_ast ? this->loc_ast->token.sourceloc
                            : this->loc_token.sourceloc;

  auto src = loc.ref;

  auto linenum = loc.line.index + 1;

  auto pathstr = src->path + ":" + std::to_string(linenum) + ":" +
                 std::to_string(loc.pos_in_line);

  std::cout << COL_BOLD COL_WHITE << pathstr << ": "
            << (as_warn ? COL_MAGENTA "warning: " : COL_RED "error: ")
            << this->msg << std::endl;

  std::cout << COL_CYAN << std::string(pathstr.length(), '-')
            << std::endl;

  if (loc.line.index >= 1) {
    std::cout << utils::Format(COL_UNBOLD COL_GRAY
                               "% 5d " COL_BOLD COL_CYAN
                               "| " COL_UNBOLD COL_GRAY,
                               linenum - 1)

              << src->GetLineView(
                     src->line_range_list[loc.line.index - 1])

              << std::endl;
  }
  else {
    std::cout << COL_CYAN "      |\n";
  }

  std::cout << utils::Format(COL_DEFAULT COL_BOLD COL_WHITE
                             "% 5d " COL_CYAN "| " COL_WHITE,
                             linenum)
            << loc.GetLine() << std::endl;

  // std::cout << COL_CYAN "      | "
  //           << std::string(loc.pos_in_line, ' ') << COL_RED "^"
  //           << std::endl;

  if (loc.line.index + 1 < src->line_range_list.size()) {
    std::cout << utils::Format(COL_UNBOLD COL_GRAY
                               "% 5d " COL_BOLD COL_CYAN
                               "| " COL_UNBOLD COL_GRAY,
                               linenum + 1);

    auto line = std::string(
        src->GetLineView(src->line_range_list[loc.line.index + 1]));

    if (line.length() <= loc.pos_in_line) {
      line += std::string(loc.pos_in_line - line.length(), ' ') +
              COL_DEFAULT COL_BOLD COL_RED "^";
    }
    else {
      line = line.substr(0, loc.pos_in_line) +
             COL_DEFAULT COL_RED COL_BOLD "^" COL_GRAY COL_UNBOLD +
             line.substr(loc.pos_in_line + 1);
    }

    std::cout << line << std::endl;
  }
  else {
    std::cout << COL_CYAN "      | "
              << std::string(loc.pos_in_line, ' ')
              << COL_RED COL_BOLD "^" << COL_DEFAULT << std::endl;
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