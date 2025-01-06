#pragma once

#include "typedef.h"
#include "SourceStorage.h"

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

//
// -----------
//  TokenKwdKind:
//    kinds for keywords.
//
enum class TokenKwdKind : u16 {
  None,

  //
  // Define function or user-defined types
  //
  Func,   // "fn"
  Enum,   // "enum"
  Class,  // "class"
  Struct, // "struct"

  //
  // Namespace.
  //
  Namespace, // "namespace"

  //
  // Define variable
  //
  Let, // "let"

  //
  // Qualifiers for let-stmt
  //
  Mut, // "mut"
  Ref, // "ref"

  In, // "in"

  //
  // Statements.
  //
  If,      // "if"
  Else,    // "else"
  Switch,  // "switch"
  Case,    // "case"
  Default, // "default"
  Match,   // "match"
  For,     // "for"
  Loop,    // "loop"
  Do,      // "do"
  While,   // "while"

  //
  // Control-keywords.
  //
  Return,   // "return"
  Break,    // "break"
  Continue, // "continue"

  //
  // Operators
  //
  Not,  // "not"
  And,  // "and"
  Or,   // "or"
  Cast, // "cast"

  //
  // Boolean
  //
  True,  // "true"
  False, // "false"

  //
  // Primitive types
  //
  Int,     // "int"
  Float,   // "float"
  Bool,    // "bool"
  Char,    // "char"
  String,  // "string"
  Vector,  // "vector"
  Tuple,   // "tuple"
  Dict,    // "dict"
  Functor, // "func"
};

//
// -----------
//  TokenOperatorKind:
//    kinds for operators.
//
enum class TokenOperatorKind : u16 {
  None,

  MemberAccess,      // .
  SubscriptionOpen,  // [
  SubscriptionClose, // ]

  Add,    // +
  Sub,    // -
  Mul,    // *
  Div,    // /
  Mod,    // %
  Assign, // =

  LShift, // <<
  RShift, // >>

  LeftBig,      // >
  RightBig,     // <
  LeftBigOrEq,  // >=
  RightBigOrEq, // <=

  Equal,    // ==
  NotEqual, // !=

  BitAnd, // &
  BitOr,  // |
  BitXor, // ^

  BitAndAssign, // &=
  BitOrAssign,  // |=
  BitXorAssign, // ^=

  LShiftAssign, // <<=
  RShiftAssign, // >>=

  AddAssign, // +=
  SubAssign, // -=
  MulAssign, // *=
  DivAssign, // /=
  ModAssign, // %=
};

//
// -----------
//  TokenPunctKind:
//    kinds for punctuators.
//
enum class TokenPunctKind : u8 {
  None,

  Comma, // ,
  Dot,   // .

  Semi,  // ;
  Colon, // :

  ScopeResol, // ::

  ResultTypeSpecifier, // ->

  CaseMatch, // =>

  BraceOpen,       // (
  BraceClose,      // )
  BlockBraceOpen,  // {
  BlockBraceClose, // }
  AngleBraceOpen,  // <
  AngleBraceClose, // >
  ArrayBraceOpen,  // [
  ArrayBraceClose, // ]

  AttributeBegin, // [[
  AttributeEnd,   // ]]

  Ellipsis, // ...
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
