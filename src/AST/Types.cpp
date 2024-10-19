#include "AST.h"
#include "alert.h"

namespace fire::AST {

ASTPtr<TypeName> TypeName::New(Token nametok) {
  return ASTNew<TypeName>(nametok);
}

TypeName::TypeName(Token name)
    : Named(ASTKind::TypeName, name),
      is_const(false) {
}

ASTPtr<Argument> Argument::New(Token nametok, ASTPtr<TypeName> type) {
  return ASTNew<Argument>(nametok, type);
}

ASTPointer Argument::Clone() const {
  return New(this->name,
             ASTCast<AST::TypeName>(this->type ? this->type->Clone() : nullptr));
}

Argument::Argument(Token nametok, ASTPtr<TypeName> type)
    : Named(ASTKind::Argument, nametok),
      type(type) {
}

ASTPtr<Enum> Enum::New(Token tok, Token name) {
  return ASTNew<Enum>(tok, name);
}

ASTPointer Enum::Clone() const {
  auto x = New(this->token, this->name);

  x->_Copy(*this);

  for (auto&& e : this->enumerators)
    x->append(e);

  return x;
}

Enum::Enum(Token tok, Token name)
    : Templatable(ASTKind::Enum, tok, name) {
}

ASTPtr<Class> Class::New(Token tok, Token name) {
  return ASTNew<Class>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name, ASTVec<VarDef> member_variables,
                         ASTVec<Function> member_functions) {
  return ASTNew<Class>(tok, name, std::move(member_variables),
                       std::move(member_functions));
}

ASTPtr<VarDef>& Class::append_var(ASTPtr<VarDef> ast) {
  return this->member_variables.emplace_back(ast);
}

ASTPtr<Function>& Class::append_func(ASTPtr<Function> ast) {
  return this->member_functions.emplace_back(ast);
}

ASTPointer Class::Clone() const {
  auto x = New(this->token, this->name, CloneASTVec<VarDef>(this->member_variables),
               CloneASTVec<Function>(this->member_functions));

  x->_Copy(*this);

  return x;
}

Class::Class(Token tok, Token name, ASTVec<VarDef> member_variables,
             ASTVec<Function> member_functions)
    : Templatable(ASTKind::Class, tok, name),
      member_variables(std::move(member_variables)),
      member_functions(std::move(member_functions)) {
}

ASTPointer TypeName::Clone() const {
  auto x = New(this->name);

  for (auto&& p : this->type_params)
    x->type_params.emplace_back(ASTCast<TypeName>(p->Clone()));

  x->is_const = this->is_const;

  x->type = this->type;

  if (this->ast_class)
    x->ast_class = ASTCast<AST::Class>(this->ast_class->Clone());

  return x;
}

ASTPtr<Signature> Signature::New(Token tok, ASTVec<TypeName> arg_type_list,
                                 ASTPtr<TypeName> result_type) {
  return ASTNew<Signature>(tok, std::move(arg_type_list), result_type);
}

ASTPointer Signature::Clone() const {
  return New(this->token, CloneASTVec<TypeName>(this->arg_type_list),
             this->result_type ? ASTCast<AST::TypeName>(this->result_type->Clone())
                               : nullptr);
}

Signature::Signature(Token tok, ASTVec<TypeName> arg_type_list,
                     ASTPtr<TypeName> result_type)
    : Base(ASTKind::Signature, tok),
      arg_type_list(std::move(arg_type_list)),
      result_type(result_type) {
}

} // namespace fire::AST