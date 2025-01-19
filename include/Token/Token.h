#pragma once

#include "SourceStorage.h"

#include "Keywords.h"
#include "Operators.h"
#include "Punctuators.h"

namespace fire {

//
// TokenKind: kinds of token
//
enum class TokenKind : u8 {
  Unknown,

  //
  // Literals
  Decimal,     // 123
  Float,       // 123.456
  Hexadecimal, // 0x123
  Binary,      // 0b1010
  String,      // "hello"
  Character,   // 'c'
  Boolean,     // true, false

  //
  // Identifiers
  Identifier,

  //
  // Punctuators
  Punctuator,
  Semi,

  //
  // Operators
  Operator, // => TokenOperatorKind

  // end of link-list
  End,
};

///
/// Token: struct for token.
///
struct Token {
  TokenKind kind;
  TokenKwdKind kwd;
  TokenOperatorKind op;
  TokenPunctKind punct;

  Token* prev;
  Token* next;
  string str;

  shared_ptr<SourceLoc> ref; // Ref to part of source file.

  // data for literal kinds
  union {
    i64 v_int;
    u64 v_hex;
    u64 v_bin;
    f64 v_float;
    char16_t v_char;
    bool v_bool;
  } literal_data;

  std::u16string v_str; // => TokenKind::String

  bool is(TokenKind k) const;
  bool is_kwd(TokenKwdKind k) const;
  bool is_op(TokenOperatorKind k) const;
  bool is_punct(TokenPunctKind k) const;
  bool is_semi() const;

  template <typename... Args>
  bool is(TokenKind k1, TokenKind k2, Args&&... args) {
    return this->is(k1) || this->is(k2, std::forward<Args>(args)...);
  }

  static Token* make(TokenKind kind, SourceStorage const* SS, Token* prev,
                     string const& str, size_t pos, TokenKwdKind kwd = TokenKwdKind::None,
                     TokenOperatorKind op = TokenOperatorKind::None,
                     TokenPunctKind punct = TokenPunctKind::None);

  static Token* make(TokenKind kind = TokenKind::Unknown);

  static string kind_to_str(TokenKind k);
  static string kwd_to_str(TokenKwdKind k);
  static string op_to_str(TokenOperatorKind k);
  static string punct_to_str(TokenPunctKind k);

  Token* set_kwd(TokenKwdKind k);
  Token* set_op(TokenOperatorKind k);
  Token* set_punct(TokenPunctKind k);

  Token* clone() const;

  Token* insert(Token* tok) {
    tok->prev = this;
    tok->next = this->next;

    this->next = tok;

    return this->next;
  }

  Token(TokenKind kind, SourceStorage const* SS, Token* prev, string const& str,
        size_t pos, TokenKwdKind kwd, TokenOperatorKind op, TokenPunctKind punct);

  Token(TokenKind kind = TokenKind::Unknown);
};

} // namespace fire