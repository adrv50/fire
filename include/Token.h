#pragma once

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
  Keyword,
  CommentLine,
  CommentBlock,
  Punctuater,
};

struct Token {
  TokenKind kind;
  std::string str;
  SourceLocation sourceloc;

  bool operator==(Token const& tok) const {
    return this->kind == tok.kind && this->str == tok.str;
  }

  Token(TokenKind kind, std::string const& str,
        SourceLocation sourceloc = {})
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