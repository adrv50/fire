#pragma once

namespace fire {

enum class ASTKind {
  Value,

  Identifier, // => AST::Identifier
  ScopeResol, // => AST::ScopeResol

  //
  // /--------------
  //  replaced to this kind from Identifier or ScopeResol in
  //  Semantics-Checker. Don't use this.
  Variable,

  MemberVariable,
  MemberFunction,

  BuiltinMemberVariable,
  BuiltinMemberFunction,

  FuncName,
  BuiltinFuncName,

  Enumerator,
  EnumName,

  ClassName,
  // ------------------/

  LambdaFunc, // => AST::Function

  OverloadResolutionGuide, // "of"

  Array,

  IndexRef,

  MemberAccess,

  RefMemberVar,

  CallFunc,
  CallFunc_Ctor,
  CallFunc_Enumerator,

  //
  // in call-func expr.
  SpecifyArgumentName, // => AST::Expr

  Not, // => todo impl.

  Mul,
  Div,
  Mod,

  Add,
  Sub,

  LShift,
  RShift,

  //
  // Compare
  //
  Bigger,        // a > b
  BiggerOrEqual, // a >= b
  Equal,
  // NotEqual  => replace: (a != b) -> (!(a == b))

  BitAND,
  BitXOR,
  BitOR,

  LogAND,
  LogOR,

  Assign,

  Block,
  Vardef,

  If,

  Match,

  Switch,
  While,

  Break,
  Continue,

  Return,

  Throw,
  TryCatch,

  Argument,
  Function,

  Enum,
  Class,

  Namespace,

  TypeName,

  Signature,
};

} // namespace fire