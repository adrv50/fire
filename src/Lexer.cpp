#include "alert.h"

#include "Parser.h"
#include "Utils.h"
#include "Error.h"

namespace fire {

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
  "none",
  "int",
  "float",
  "usize",
  "bool",
  "char",
  "string",
  "vector",
  "tuple",
  "dict",
  "Module",
  "Func",

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
    : source(source),
      position(0),
      length(source.data.length()) {
}

TokenVector Lexer::Lex() {
  TokenVector vec;

  this->pass_space();

  while (this->check()) {
    auto c = this->peek();
    auto s = this->source.data.data() + this->position;
    auto pos = this->position;

    auto& tok = vec.emplace_back();

    tok.str = " ";
    tok.sourceloc = SourceLocation(pos, 1, &this->source);

    // hex
    if (this->eat("0x") || this->eat("0X")) {
      tok.kind = TokenKind::Hex;

      while (isxdigit(this->peek()))
        this->position++;
    }

    // bin
    else if (this->eat("0b") || this->eat("0B")) {
      tok.kind = TokenKind::Bin;

      while (this->peek() == '0' || this->peek() == '1')
        this->position++;
    }

    // digits
    else if (isdigit(c)) {
      tok.kind = TokenKind::Int;

      while (isdigit(this->peek()))
        this->position++;

      // float
      if (this->eat(".")) {
        tok.kind = TokenKind::Float;

        while (isdigit(this->peek()))
          this->position++;
      }

      if (this->eat("f"))
        tok.kind = TokenKind::Float;
    }

    // identifier or keyword
    else if (isalpha(c) || c == '_') {
      tok.kind = TokenKind::Identifier;

      while (isalnum(this->peek()) || this->peek() == '_')
        this->position++;
    }

    // char or string literal
    else if (this->eat('\'') || this->eat('"')) {
      tok.kind = c == '"' ? TokenKind::String : TokenKind::Char;

      while (this->check() && this->peek() != c)
        this->position++;

      if (!this->check() || !this->eat(c)) {
        Error(tok).format("not terminated %s literal",
                          c == '"' ? "string" : "character")();
      }
    }

    else {
      for (std::string_view s : punctuaters) {
        if (this->match(s)) {
          tok.kind = TokenKind::Punctuater;
          tok.str = s;
          this->position += s.length();
          goto found_punct;
        }
      }

      Error(tok, "invalid token")();
    }

    tok.str = std::string_view(s, this->position - pos);

    if (tok.str == "true" || tok.str == "false")
      tok.kind = TokenKind::Boolean;
    else if (tok.kind == TokenKind::Identifier && is_keyword(tok.str))
      tok.kind = TokenKind::Keyword;

  found_punct:
    tok.sourceloc =
        SourceLocation(pos, tok.str.length(), &this->source);

    this->pass_space();
  }

  return vec;
}

bool Lexer::check() const {
  return this->position < this->length;
}

char Lexer::peek() {
  return this->source[this->position];
}

void Lexer::pass_space() {
  while (isspace(this->peek()))
    this->position++;
}

bool Lexer::match(std::string_view str) {
  return this->position + str.length() <= this->length &&
         this->trim(str.length()) == str;
}

} // namespace fire
