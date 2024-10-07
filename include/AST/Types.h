#pragma once

namespace fire::AST {

struct TypeName : Named {
  ASTPtr<Signature> sig;

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

struct Enum : Templatable {
  struct Enumerator {
    Token name;
    ASTPtr<TypeName> type = nullptr;
  };

  vector<Enumerator> enumerators;

  Enumerator& append(Token name, ASTPtr<TypeName> type);
  Enumerator& append(Enumerator const& e);

  static ASTPtr<Enum> New(Token tok, Token name);

  ASTPointer Clone() const override;

  Enum(Token tok, Token name);
};

struct Class : Templatable {
  ASTVec<VarDef> member_variables;
  ASTVec<Function> member_functions;

  static ASTPtr<Class> New(Token tok, Token name);

  static ASTPtr<Class> New(Token tok, Token name, ASTVec<VarDef> member_variables,
                           ASTVec<Function> member_functions);

  ASTPtr<VarDef>& append_var(ASTPtr<VarDef> ast);

  ASTPtr<Function>& append_func(ASTPtr<Function> ast);

  ASTPointer Clone() const override;

  Class(Token tok, Token name, ASTVec<VarDef> member_variables = {},
        ASTVec<Function> member_functions = {});
};

}