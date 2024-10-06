#pragma once

#include <memory>

#include "Object.h"
#include "Source.h"

namespace fire {

enum class TokenKind : u8 {
  Unknown,
  Int,
  Hex,
  Bin,
  Float,
  Size,
  Boolean,
  Char,
  String,
  Identifier,
  CommentLine,
  CommentBlock,
  Punctuater,
};

struct Token {
  i64 _index = 0;

  TokenKind kind;
  std::string_view str;
  SourceLocation sourceloc;

  Token const* get_backword(int step = 1);
  Token const* get_forword(int step = 1);

  string& get_src_data() {
    return this->sourceloc.ref->data;
  }

  bool operator==(Token const& tok) const {
    return this->kind == tok.kind && this->str == tok.str;
  }

  Token(TokenKind kind, std::string const& str, SourceLocation sourceloc = {})
      : kind(kind),
        str(str),
        sourceloc(sourceloc) {
  }

  Token(std::string str)
      : Token(TokenKind::Unknown, str) {
  }

  Token(char const* str = "")
      : Token(std::string(str)) {
  }

  Token(TokenKind kind)
      : Token(kind, "") {
  }
};

} // namespace fire