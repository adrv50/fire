#include "alert.h"

#include "Parser.h"
#include "Utils.h"
#include "Error.h"

namespace metro {

// clang-format off
static char const* punctuaters[] = {
    "<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "..",
    "&&",  "||",  "->", "::", "<",  ">",  "+",  "-",  "/",
    "*",   "%",   "=",  ";",  ":",  ",",  ".",  "[",  "]",
    "(",   ")",   "{",  "}",  "!",  "?",  "&",  "^",  "|",
};

static char const* keywords[] = {
  //
  // built-in type names
  "int",
  "float",
  "usize",
  "bool",
  "char",
  "string",
  "vector",
  "tuple",
  "dict",
  "module",
  "function",

  "enum",
  "class",
  "fn",

  "let",
  "if",
  "switch",
  "case",
  "match",  // feature
  "loop",
  "for",
  "do",
  "while",

  "getid",
  "typeof",
};
// clang-format on

static bool is_keyword(std::string_view str) {
  for (auto&& kwd : keywords)
    if (str == kwd)
      return true;

  return false;
}

Lexer::Lexer(SourceStorage const& source)
    : source(source), pos(0), length(source.data.length()) {
}

TokenVector Lexer::Lex() {
  TokenVector vec;

  this->pass_space();

  while (this->check()) {
    auto c = this->peek();
    auto s = this->source.data.data() + this->pos;
    auto pos = this->pos;

    auto& tok = vec.emplace_back();

    tok.str = " ";
    tok.sourceloc = SourceLocation(pos, 1, &this->source);

    // hex
    if (this->match("0x") || this->match("0X")) {
    }

    // bin
    else if (this->match("0x") || this->match("0X")) {
    }

    // digits
    else if (isdigit(c)) {
      tok.kind = TokenKind::Int;

      while (isdigit(this->peek()))
        this->pos++;
    }

    // identifier or keyword
    else if (isalpha(c) || c == '_') {
      tok.kind = TokenKind::Identifier;

      while (isalnum(this->peek()) || this->peek() == '_')
        this->pos++;
    }

    else if (c == '\'' || c == '"') {
      tok.kind = c == '"' ? TokenKind::String : TokenKind::Char;

      for (this->pos++; this->peek() != c; this->pos++)
        ;

      this->pos++;

      if (!this->check()) {
        Error(tok,
              "not terminated " +
                  std::string(c == '"' ? "string" : "character") +
                  " literal")();
      }

      if (c == '\'') {
        auto len = this->pos - pos - 2;

        if (len == 0 || len >= 2)
          Error(tok, "invalid character literal")();
      }
    }

    else {
      for (std::string_view s : punctuaters) {
        if (this->match(s)) {
          tok.kind = TokenKind::Punctuater;
          tok.str = s;
          this->pos += s.length();
          goto found_punct;
        }
      }

      Error(tok, "invalid token")();
    }

    tok.str = std::string_view(s, this->pos - pos);

    if (tok.kind == TokenKind::Identifier && is_keyword(tok.str))
      tok.kind = TokenKind::Keyword;

  found_punct:
    tok.sourceloc =
        SourceLocation(pos, tok.str.length(), &this->source);

    this->pass_space();
  }

  return vec;
}

bool Lexer::check() const {
  return this->pos < this->length;
}

char Lexer::peek() {
  return this->source[this->pos];
}

void Lexer::pass_space() {
  while (isspace(this->peek()))
    this->pos++;
}

bool Lexer::match(std::string_view str) {
  return this->pos + str.length() <= this->length &&
         this->trim(str.length()) == str;
}

} // namespace metro
