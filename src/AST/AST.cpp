#include "AST.h"
#include "alert.h"

namespace fire::AST {

std::pair<ASTKind, char const*> const ASTKindNameArray[] = {
    {ASTKind::Value, "Value"},
    {ASTKind::Identifier, "Identifier"},
    {ASTKind::ScopeResol, "ScopeResol"},
    {ASTKind::Variable, "Variable"},
    {ASTKind::MemberVariable, "MemberVariable"},
    {ASTKind::MemberFunction, "MemberFunction"},
    {ASTKind::BuiltinMemberVariable, "BuiltinMemberVariable"},
    {ASTKind::BuiltinMemberFunction, "BuiltinMemberFunction"},
    {ASTKind::FuncName, "FuncName"},
    {ASTKind::BuiltinFuncName, "BuiltinFuncName"},
    {ASTKind::Enumerator, "Enumerator"},
    {ASTKind::EnumName, "EnumName"},
    {ASTKind::ClassName, "ClassName"},
    {ASTKind::LambdaFunc, "LambdaFunc"},
    {ASTKind::OverloadResolutionGuide, "OverloadResolutionGuide"},
    {ASTKind::Array, "Array"},
    {ASTKind::IndexRef, "IndexRef"},
    {ASTKind::MemberAccess, "MemberAccess"},
    {ASTKind::RefMemberVar, "RefMemberVar"},
    {ASTKind::RefMemberVar_Left, "RefMemberVar_Left"},
    {ASTKind::CallFunc, "CallFunc"},
    {ASTKind::CallFunc_Ctor, "CallFunc_Ctor"},
    {ASTKind::CallFunc_Enumerator, "CallFunc_Enumerator"},
    {ASTKind::SpecifyArgumentName, "SpecifyArgumentName"},
    {ASTKind::Not, "Not"},
    {ASTKind::Mul, "Mul"},
    {ASTKind::Div, "Div"},
    {ASTKind::Mod, "Mod"},
    {ASTKind::Add, "Add"},
    {ASTKind::Sub, "Sub"},
    {ASTKind::LShift, "LShift"},
    {ASTKind::RShift, "RShift"},
    {ASTKind::Bigger, "Bigger"},
    {ASTKind::BiggerOrEqual, "BiggerOrEqual"},
    {ASTKind::Equal, "Equal"},
    {ASTKind::BitAND, "BitAND"},
    {ASTKind::BitXOR, "BitXOR"},
    {ASTKind::BitOR, "BitOR"},
    {ASTKind::LogAND, "LogAND"},
    {ASTKind::LogOR, "LogOR"},
    {ASTKind::Assign, "Assign"},
    {ASTKind::Block, "Block"},
    {ASTKind::Vardef, "Vardef"},
    {ASTKind::If, "If"},
    {ASTKind::Match, "Match"},
    {ASTKind::Switch, "Switch"},
    {ASTKind::While, "While"},
    {ASTKind::Break, "Break"},
    {ASTKind::Continue, "Continue"},
    {ASTKind::Return, "Return"},
    {ASTKind::Throw, "Throw"},
    {ASTKind::TryCatch, "TryCatch"},
    {ASTKind::Argument, "Argument"},
    {ASTKind::Function, "Function"},
    {ASTKind::Enum, "Enum"},
    {ASTKind::Class, "Class"},
    {ASTKind::Namespace, "Namespace"},
    {ASTKind::TypeName, "TypeName"},
    {ASTKind::Signature, "Signature"},
};

char const* GetKindStr(ASTKind const K) {
#if _METRO_DEBUG_
  for (auto&& [k, s] : ASTKindNameArray)
    if (k == K)
      return s;
  panic;
#endif

  return ASTKindNameArray[static_cast<int>(K)].second;
}

ASTPtr<Identifier> GetID(ASTPointer ast) {

  if (ast->IsConstructedAs(ASTKind::MemberAccess))
    return GetID(ast->as_expr()->rhs);

  if (ast->IsConstructedAs(ASTKind::ScopeResol))
    return ast->As<AST::ScopeResol>()->GetLastID();

  assert(ast->IsConstructedAs(ASTKind::Identifier));

  return ASTCast<AST::Identifier>(ast);
}

bool Base::IsQualifiedIdentifier() {
  return this->IsIdentifier() && this->GetID()->id_params.size() >= 1;
}

bool Base::IsUnqualifiedIdentifier() {
  return this->IsIdentifier() && this->GetID()->id_params.empty();
}

Expr const* Base::as_expr() const {
  return static_cast<Expr const*>(this);
}

Statement const* Base::as_stmt() const {
  return static_cast<Statement const*>(this);
}

Expr* Base::as_expr() {
  return static_cast<Expr*>(this);
}

Statement* Base::as_stmt() {
  return static_cast<Statement*>(this);
}

Value const* Base::as_value() const {
  return (Value const*)this;
}

Identifier* Base::GetID() {
  return nullptr;
}

i64 Base::GetChilds(ASTVector& out) const {

  (void)out;
  todo_impl;

  return 0;
}

// ----------------------------------- //

} // namespace fire::AST
