#pragma once

#include "typedef.h"

namespace fire {

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

  Slash,            // /   #
  PathParentFolder, // ..  # for "import" statement

  Ellipsis, // ...
};

} // namespace fire