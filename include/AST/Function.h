#pragma once

namespace fire::AST {

struct Argument : Named {
  ASTPtr<TypeName> type;

  static ASTPtr<Argument> New(Token nametok, ASTPtr<TypeName> type);

  ASTPointer Clone() const;

  Argument(Token nametok, ASTPtr<TypeName> type);
};

struct Function : Templatable {
  ASTVec<Argument> arguments;
  ASTPtr<TypeName> return_type;
  ASTPtr<Block> block;

  bool is_var_arg = false;

  ASTPtr<Class> member_of = nullptr;

  bool is_virtualized = false;
  Token virtualize_specify_tok;

  bool is_override = false;
  Token override_specify_tok;

  ASTPtr<Class> OverridingClass;   //
  ASTPtr<Function> OverridingFunc; // --> base-class

  sema::FunctionScope* GetScope();

  static ASTPtr<Function> New(Token tok, Token name);

  static ASTPtr<Function> New(Token tok, Token name, ASTVec<Argument> args,
                              bool is_var_arg, ASTPtr<TypeName> rettype,
                              ASTPtr<Block> block);

  ASTPtr<Argument>& add_arg(Token const& tok, ASTPtr<TypeName> type = nullptr);

  ASTPtr<Argument> find_arg(std::string const& name);

  ASTPointer Clone() const override;

  Function(Token tok, Token name);
  Function(Token tok, Token name, ASTVec<Argument> args, bool is_var_arg,
           ASTPtr<TypeName> rettype, ASTPtr<Block> block);
};

} // namespace fire::AST