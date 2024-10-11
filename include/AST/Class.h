#pragma once

#include <concepts>

namespace fire::AST {

struct Class : Templatable {
  // parse
  ASTVector derive_names; // elem is Parser::ScopeResol()

  ASTVec<Class> derived_classes; //
  ASTVec<Class> BasedBy;         // --> Sema

  ASTVec<VarDef> member_variables;
  ASTVec<Function> member_functions;

  bool IsFinal = false;
  Token FianlSpecifyToken;

  static ASTPtr<Class> New(Token tok, Token name);

  static ASTPtr<Class> New(Token tok, Token name, ASTVec<VarDef> member_variables,
                           ASTVec<Function> member_functions);

  ASTPtr<VarDef>& append_var(ASTPtr<VarDef> ast);

  ASTPtr<Function>& append_func(ASTPtr<Function> ast);

  ASTPointer Clone() const override;

  Class(Token tok, Token name, ASTVec<VarDef> member_variables = {},
        ASTVec<Function> member_functions = {});
};

} // namespace fire::AST