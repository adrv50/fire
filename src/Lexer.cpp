#include "alert.h"

#include "Lexer.h"
#include "Utils.h"
#include "Error.h"

namespace fire {

static char const* punctuaters[] = {
    "...", "<<=", ">>=", "<<", ">>", "=>", "<=", ">=", "==", "!=", "..", "+=",
    "-=",  "*=",  "/=",  "%=", "&=", "^=", "|=", "&&", "||", "->", "::", "<",
    ">",   "+",   "-",   "/",  "*",  "%",  "=",  ";",  ":",  ",",  ".",  "[",
    "]",   "(",   ")",   "{",  "}",  "!",  "?",  "&",  "^",  "|",  "@",
};

Lexer::Lexer(SourceStorage& source)
    : source(source),
      position(0),
      length(source.data.length()),
      src_view(source.data) {
}

bool Lexer::Lex(Vec<Token>& out) {
  this->pass_space();

  ///
  /// パーサで，テンプレートパラメータもしくはテンプレート引数のパースにおいて
  /// ">"
  /// が２個連続になっていて右シフト演算子になっている場合，それを分割する操作をトークン配列に対して行います．
  /// メモリ再確保によって Token
  /// ポインタが無効にならないよう，字句解析で登場する右シフト演算子の回数をここに加算し，最後にトークンベクタのサイズを，これを足した数で
  /// reserve() を行います．
  size_t BufCount_for_TemplParamCloseTokenSplitting = 0;

  while (this->check()) {
    auto c = this->peek();
    auto s = this->source.data.data() + this->position;
    auto pos = this->position;

    // comment line
    if (this->eat("//")) {
      while (this->check() && !this->eat('\n'))
        this->position++;

      this->pass_space();
      continue;
    }

    // comment block
    if (this->eat("/*")) {
      while (this->check() && !this->eat("*/"))
        this->position++;

      this->pass_space();
      continue;
    }

    Token& tok = out.emplace_back(
        Token(TokenKind::Unknown, " ", SourceLocation(pos, 1, &this->source)));

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
      char quat = c;
      bool is_str = quat == '"';

      tok.kind = is_str ? TokenKind::String : TokenKind::Char;

      while (this->check() && (c = this->peek()) != quat)
        this->position++;

      if (!this->check() || !this->eat(quat)) {
        throw Error(tok).format("not terminated %s literal",
                                is_str ? "string" : "character");
      }
    }

    else {
      for (std::string_view s : punctuaters) {
        if (this->match(s)) {
          tok.kind = TokenKind::Punctuater;
          tok.str = s;

          if (s == ">>")
            BufCount_for_TemplParamCloseTokenSplitting++;

          this->position += s.length();
          goto found_punct;
        }
      }

      throw Error(tok, "invalid token");
    }

    tok.str = std::string_view(s, this->position - pos);

    if (tok.str == "true" || tok.str == "false")
      tok.kind = TokenKind::Boolean;

  found_punct:

    // tok.sourceloc = SourceLocation(pos, tok.str.length(), &this->source);
    tok.sourceloc.length = this->position - pos;

    this->pass_space();
  }

  out.reserve(out.size() + BufCount_for_TemplParamCloseTokenSplitting +
              1); // +1 is insurance.

  return true;
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

  i64 const len = static_cast<i64>(str.length());

  return this->position + len <= this->length &&
         this->src_view.substr(this->position, len) == str;
}

} // namespace fire
