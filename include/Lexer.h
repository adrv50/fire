#pragma once

#include "typedef.h"

namespace fire {

class SourceStorage;
struct Token;

class Lexer {

  SourceStorage& SS;

  size_t pos;
  size_t const len;

  bool check(int add = 1) const;

  char peek(int offset = 0) const;

  string_view get(int len) const;

  bool eat(string_view s, bool keep_pos = false);
  bool match(string_view s);

  string trim_hexadecimal();
  string trim_binary();
  string trim_decimal();
  string trim_identifier();

  void pass_space();

public:
  Lexer(SourceStorage& SS);

  //
  // do lex
  //
  Token* lex();
};

} // namespace fire