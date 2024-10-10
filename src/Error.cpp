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
    alertexpr(src);
    alertexpr(index);
    alertexpr(pos);
    alertexpr(linenum);

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

static char const* get_err_level_str(Error::ErrorKind k) {
  switch (k) {
  case Error::ER_Error:
    return COL_ERROR "error: ";

  case Error::ER_Warning:
    return COL_WARNING "warning: ";

  case Error::ER_Note:
    return COL_NOTE "note: ";
  }

  return nullptr;
}

Error const& Error::emit() const {

  using std::cout;
  using std::endl;

  Token const& err_token = this->loc_ast ? this->loc_ast->token : this->loc_token;

  line_data_wrapper_t line_top, line_bottom, line_err = line_data_wrapper_t(err_token);

  line_err.get_other_line(line_top, -1);
  line_err.get_other_line(line_bottom, 1);

  SourceLocation const& loc = err_token.sourceloc;

  std::stringstream ss;

  // location
  for (auto&& _loc : this->location) {
    cout << COL_BOLD << loc.ref->path << ": " << _loc << ":" << endl;
  }

  // message
  cout << COL_BOLD << get_err_level_str(this->kind) << COL_WHITE << this->msg << endl;

  // path and location
  cout << "     " << COL_BOLD COL_UNDERLINE COL_BORDER_LINE << "--> " << loc.ref->path
       << ":" << line_err.linenum << ":" << line_err.pos << COL_DEFAULT << endl;

  cout << COL_DEFAULT << COL_BOLD << "     " COL_BORDER_LINE " |" << endl;

  cout << COL_DEFAULT << COL_BOLD << utils::Format("%5d", line_err.linenum)
       << COL_BORDER_LINE " | " << COL_WHITE << line_err.view << endl;

  cout << COL_DEFAULT << COL_BOLD << "     " COL_BORDER_LINE " | ";

  for (int i = 0; i < line_err.pos;) {
    u8 c = line_err.view[i];

    if (c <= 0x80) {
      cout << ' ';
      i += 1;
    }
    else if (0xC2 <= c && c < 0xE0) {
      cout << "\xe3\x80\x80";
      i += 2;
    }
    else if (0xE0 <= c && c < 0xF0) {
      cout << "\xe3\x80\x80";
      i += 3;
    }
    else if (0xF0 <= c && c < 0xF5) {
      cout << "\xe3\x80\x80";
      i += 4;
    }
  }

  cout << COL_WHITE COL_BOLD "^" << COL_DEFAULT << endl;

  _err_emitted_count++;

  for (auto&& e : this->chained)
    e.emit();

  for (auto&& note : this->notes) {
    cout << "     " << COL_DEFAULT COL_BOLD COL_NOTE << "note: " << COL_WHITE << note
         << COL_DEFAULT << endl;
  }

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