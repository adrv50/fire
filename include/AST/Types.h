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

struct Enum : Templatable {
  struct Enumerator {
    enum class DataType {
      NoData,
      Value,     // Kind(T)
      Structure, // Kind(a: T, b: U)
    };

    Token name;

    DataType data_type = DataType::NoData;

    //
    // usage of types:
    //   NoData     => (empty)
    //   Value      => types[0] is ASTPtr<TypeName>
    //   Structure  => types is ASTVec<Argument>
    ASTVector types;
  };

  vector<Enumerator> enumerators;

  template <typename... Args>
  requires std::constructible_from<Enumerator, Args...>
  Enumerator& append(Args&&... args) {
    return this->enumerators.emplace_back(std::forward<Args>(args)...);
  }

  static ASTPtr<Enum> New(Token tok, Token name);

  ASTPointer Clone() const override;

  Enum(Token tok, Token name);
};

struct Class : Templatable {
  ASTVector derive_names;

  ASTVec<Class> derived_classes; // --> Sema

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

} // namespace fire::AST