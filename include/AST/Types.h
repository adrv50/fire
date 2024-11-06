#pragma once

#include <concepts>

namespace fire::AST {

struct TypeName : Named {
  ASTPtr<Signature> sig = nullptr;

  ASTVec<TypeName> type_params;
  bool is_const;

  TypeInfo type;
  ASTPtr<Class> ast_class = nullptr;

  static ASTPtr<TypeName> New(Token nametok);

  ASTPointer Clone() const override;

  TypeName(Token name);
};

struct Signature : Base {
  ASTVec<TypeName> arg_type_list;
  ASTPtr<TypeName> result_type;

  static ASTPtr<Signature> New(Token tok, ASTVec<TypeName> arg_type_list,
                               ASTPtr<TypeName> result_type);

  ASTPointer Clone() const override;

  Signature(Token tok, ASTVec<TypeName> arg_type_list, ASTPtr<TypeName> result_type);
};

} // namespace fire::AST