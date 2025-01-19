#pragma once

#include "typedef.h"

namespace fire {

//
// -----------
//  TokenKwdKind:
//    kinds for keywords.
//
enum class TokenKwdKind : u16 {
  None,

  Import, // "import"

  //
  // Define function or user-defined types
  //
  Func,   // "fn"
  Enum,   // "enum"
  Class,  // "class"
  Struct, // "struct"

  //
  // concept define
  Concept, // "concept"

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

} // namespace fire