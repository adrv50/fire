#pragma once

#include <concepts>

namespace fire::AST {

struct Interface;

struct Class : Templatable {
  ASTVec<VarDef> member_variables;
  ASTVec<Function> member_functions;

  bool IsFinal = false;
  Token FianlSpecifyToken;

  // parse
  ASTPointer InheritBaseClassName; // Parser::ScopeResol()

  ASTPtr<Class> InheritBaseClassPtr;

  ASTVec<Class> InheritedBy;

  ASTVec<Interface> CoveredInterfaces; // feature

  //
  ASTVec<Function> VirtualFunctions;

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