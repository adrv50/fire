#pragma once

#include "typedef.h"

namespace fire {



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

}