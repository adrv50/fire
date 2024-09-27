#include <iostream>

#include "alert.h"
#include "Utils.h"
#include "Color.h"

#include "Error.h"

#define COL_WARNING COL_MAGENTA
#define COL_ERROR COL_RED
#define COL_NOTE COL_YELLOW

#define COL_MESSAGE COL_BOLD COL_WHITE

#define COL_LINENUM COL_BOLD COL_WHITE
#define LINENUM_WIDTH 5

#define COL_BORDER_LINE COL_CYAN

namespace fire {

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

  bool get_other_line(line_data_wrapper_t& out, int index_diff) const {

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

      ss << utils::Format(utils::Format("%s%% %dd", is_main ? COL_LINENUM : COL_GRAY,
                                        LINENUM_WIDTH),
                          this->linenum)
         << COL_BORDER_LINE << " | "
         << (is_main ? COL_BOLD COL_WHITE : COL_UNBOLD COL_GRAY) << this->view;

      return ss.str();
    }

    return std::string(LINENUM_WIDTH, ' ') + COL_BORDER_LINE + " |";
  }
};

static int _err_emitted_count = 0;

Error const& Error::emit() const {

  Token const& err_token = this->loc_ast ? this->loc_ast->token : this->loc_token;

  line_data_wrapper_t line_top, line_bottom, line_err = line_data_wrapper_t(err_token);

  line_err.get_other_line(line_top, -1);
  line_err.get_other_line(line_bottom, 1);

  SourceLocation const& loc = err_token.sourceloc;

  std::stringstream ss;

  std::string errlvstr;

  switch (this->kind) {
  case ER_Error:
    errlvstr = COL_ERROR "error: ";
    break;

  case ER_Warning:
    errlvstr = COL_WARNING "warning: ";
    break;

  case ER_Note:
    errlvstr = COL_NOTE "note: ";
    break;
  }

  // location
  for (auto&& _loc : this->location) {
    ss << COL_BOLD << loc.ref->path << ": " << _loc << ":" << std::endl;
  }

  // message
  ss << COL_BOLD << errlvstr << COL_WHITE << this->msg << std::endl;

  // path and location
  ss << "     " << COL_BOLD COL_UNDERLINE COL_BORDER_LINE << "--> " << loc.ref->path
     << ":" << line_err.linenum << ":" << line_err.pos << COL_DEFAULT << std::endl;

  StringVector screen = {
      line_top.to_print_str(false),
      line_err.to_print_str(true),
      line_bottom.to_print_str(false),
  };

  // insert cursor '^'
  {
    auto& s = screen[2];

    auto cursorpos = line_err.pos + LINENUM_WIDTH + 3 + utils::get_color_length_in_str(s);

    if ((int)s.length() <= cursorpos)
      s += std::string(cursorpos - s.length() + 10, ' ');

    s = s.substr(0, cursorpos) + COL_DEFAULT COL_BOLD COL_WHITE "^" COL_UNBOLD COL_GRAY +
        s.substr(cursorpos + 1);
  }

  // | (and back line if got)
  ss << COL_DEFAULT << screen[0] << std::endl;

  // | error line
  ss << COL_DEFAULT << screen[1] << std::endl;

  // | (and next line if got)  and cursor
  ss << COL_DEFAULT << screen[2] << std::endl;

  for (auto&& note : this->notes) {
    ss << "     " << COL_DEFAULT COL_BOLD COL_NOTE << "note: " << COL_WHITE << note
       << COL_DEFAULT << std::endl;
  }

  std::cout << COL_DEFAULT << ss.str() << COL_DEFAULT << std::endl;

  _err_emitted_count++;

  return *this;
}

// Error& Error::emit_as_hint() {
// }

int Error::GetEmittedCount() {
  return _err_emitted_count;
}

void Error::operator()() {
  this->stop();
}

void Error::stop() {
  std::exit(1);
}

void Error::fatal_error(std::string const& msg) {
  std::cout << COL_BOLD COL_RED << "fatal error: " << COL_DEFAULT << msg << std::endl;

  std::exit(2);
}

Error::Error(ErrorKind k, Token tok, std::string msg)
    : kind(k),
      loc_token(tok),
      msg(std::move(msg)) {
}

Error::Error(ErrorKind k, ASTPointer ast, std::string msg)
    : kind(k),
      loc_ast(ast),
      msg(std::move(msg)) {
}

Error::Error(Token tok, std::string msg)
    : Error(ER_Error, tok, std::move(msg)) {
}

Error::Error(ASTPointer ast, std::string msg)
    : Error(ER_Error, ast, std::move(msg)) {
}

} // namespace fire