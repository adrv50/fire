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
  string str;
  SourceLocation sourceloc;

  Token const* get_prev(int step = 1);
  Token const* get_next(int step = 1);

  string& get_src_data() {
    return this->sourceloc.ref->data;
  }

  bool operator==(Token const& tok) const {
    return this->kind == tok.kind && this->str == tok.str;
  }

  Token(TokenKind kind, string str, SourceLocation sourceloc = {})
      : kind(kind),
        str(std::move(str)),
        sourceloc(sourceloc) {
  }

  Token(string str)
      : Token(TokenKind::Unknown, std::move(str)) {
  }

  Token(char const* str = "")
      : Token(string(str)) {
  }

  Token(TokenKind kind)
      : Token(kind, "") {
  }
};

} // namespace fire