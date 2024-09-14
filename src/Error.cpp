#include <iostream>

#include "alert.h"
#include "Utils.h"
#include "Color.h"

#include "Error.h"

#define COL_WARNING COL_MAGENTA
#define COL_ERROR COL_RED

#define COL_MESSAGE COL_BOLD COL_WHITE

#define COL_LINENUM COL_BOLD COL_WHITE
#define LINENUM_WIDTH 5

#define COL_BORDER_LINE COL_CYAN

namespace metro {

struct line_data_wrapper_t {
  SourceStorage const* src;

  int index;
  int pos; // pos on line

  std::string_view view;
  int linenum;

  line_data_wrapper_t(Token const& tok)
      : src(tok.sourceloc.ref) {
    this->index = tok.sourceloc.line.index;
    this->pos = tok.sourceloc.pos_in_line;

    this->view = tok.sourceloc.GetLine();
    this->linenum = this->index + 1;
  }

  line_data_wrapper_t()
      : src(nullptr),
        index(0),
        pos(0),
        linenum(0) {
  }

  bool get_other_line(line_data_wrapper_t& out,
                      int index_diff) const {

    int _index = this->index + index_diff;

    if (_index < 0 || _index >= this->src->Count())
      return false;

    out.src = this->src;
    out.index = _index;
    out.pos = 0;

    out.view = this->src->GetLineView(_index);
    out.linenum = out.index + 1;

    return true;
  }

  std::string to_print_str(bool is_main) const {
    if (this->src) {
      std::stringstream ss;

      ss << utils::Format(
                utils::Format("%s%% %dd",
                              is_main ? COL_LINENUM : COL_GRAY,
                              LINENUM_WIDTH),
                this->linenum)
         << COL_BORDER_LINE << " | "
         << (is_main ? COL_BOLD COL_WHITE : COL_UNBOLD COL_GRAY)
         << this->view;

      return ss.str();
    }

    return std::string(LINENUM_WIDTH, ' ') + COL_BORDER_LINE + " |";
  }
};

Error& Error::emit(bool as_warn) {

  auto& err_token =
      this->loc_ast ? this->loc_ast->token : this->loc_token;

  line_data_wrapper_t line_top, line_bottom,
      line_err = line_data_wrapper_t(err_token);

  line_err.get_other_line(line_top, -1);
  line_err.get_other_line(line_bottom, 1);

  SourceLocation& loc = err_token.sourceloc;

  std::stringstream ss;

  // path and location
  ss << COL_BOLD COL_WHITE << loc.ref->path << ":" << line_err.linenum
     << ":" << line_err.pos << ": ";

  // message
  ss << (as_warn ? COL_WARNING "warning: " : COL_ERROR "error: ")
     << COL_BOLD COL_WHITE << this->msg << std::endl;

  StringVector screen = {
      COL_BORDER_LINE + std::string(10, '-'),
      line_top.to_print_str(false),
      line_err.to_print_str(true),
      line_bottom.to_print_str(false),
  };

  // insert cursor '^'
  {
    auto& s = screen[3];

    auto cursorpos = line_err.pos + LINENUM_WIDTH + 3 +
                     utils::get_color_length_in_str(s);

    if (s.length() <= cursorpos)
      s += std::string(cursorpos - s.length() + 10, ' ');

    s = s.substr(0, cursorpos) +
        COL_DEFAULT COL_BOLD COL_WHITE "^" COL_UNBOLD COL_GRAY +
        s.substr(cursorpos + 1);
  }

  // border
  // -------
  ss << screen[0] << std::endl;

  // | (and back line if got)
  ss << COL_DEFAULT << screen[1] << std::endl;

  // | error line
  ss << COL_DEFAULT << screen[2] << std::endl;

  // | (and next line if got)  and cursor
  ss << COL_DEFAULT << screen[3] << std::endl;

  std::cout << COL_DEFAULT << ss.str() << COL_DEFAULT << std::endl;

  return *this;
}

// Error& Error::emit_as_hint() {
// }

void Error::operator()() {
  this->emit().stop();
}

[[noreturn]]
void Error::stop() {
  std::exit(1);
}

Error::Error(Token tok, std::string msg)
    : loc_token(tok),
      msg(std::move(msg)) {
}

Error::Error(ASTPointer ast, std::string msg)
    : loc_ast(ast),
      msg(std::move(msg)) {
}

} // namespace metro