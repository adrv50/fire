#pragma once

#include <string>
#include <memory>
#include "../Source.h"

namespace metro {

enum class TokenKind : u8 {
  Int,   // 123
  Float, // 3.14f
  Size,  // 100000u
  Char,
  String,
  Boolean,
  Identifier,
  Keyword,
  Punctuater,
  Unknown,
};

struct Token {
  TokenKind kind;
  std::string_view str;
  SourceLocation sourceloc;

  Token(TokenKind kind, std::string_view str) : kind(kind), str(str) {
  }

  Token(TokenKind kind = TokenKind::Unknown) : Token(kind, "") {
  }

  Token(char const* str, TokenKind kind = TokenKind::Identifier)
      : Token(kind, str) {
  }
};

using TokenVector = std::vector<Token>;
using TokenIterator = TokenVector::iterator;

} // namespace metro