#pragma once

#include "Token.h"

namespace fire {

class Lexer {

public:
  Lexer(SourceStorage const& source);

  TokenVector Lex();

private:
  bool check() const;
  char peek();
  void pass_space();
  bool match(std::string_view);

  bool eat(char c) {
    if (this->peek() == c) {
      this->position++;
      return true;
    }

    return false;
  }

  bool eat(std::string_view s) {
    if (this->match(s)) {
      this->position += s.length();
      return true;
    }

    return false;
  }

  std::string_view trim(i64 len) {
    return std::string_view(this->source.data.data() + this->position,
                            len);
  }

  SourceStorage const& source;
  i64 position;
  i64 length;
};

} // namespace fire
