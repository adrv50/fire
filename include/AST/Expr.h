#pragma once

namespace fire::AST {

struct Value : Base {
  ObjPointer value;

  static ASTPtr<Value> New(Token tok, ObjPointer val);

  ASTPointer Clone() const override;

  Value(Token tok, ObjPointer value);
};

struct Identifier : Named {
  Token paramtok; // "<"

  ASTVector id_params; // T = Identifier or ScopeResol

  // flags for Sema
  bool sema_must_completed = true;
  bool sema_allow_ambiguous = false;
  bool sema_use_keeped = false;

  //
  // for Kind::FuncName or BuiltinFuncName
  ASTVec<Function> candidates;
  vector<builtins::Function const*> candidates_builtin;

  bool template_func_decided = false;
  ASTPtr<Function> template_original;

  TypeInfo ft_ret;
  vector<TypeInfo> ft_args;

  vector<TypeInfo> template_args;

  //
  // for BuiltinMemberVariable
  builtins::MemberVariable const* blt_member_var = nullptr;

  //
  //----------------------

  //
  // for Kind::Variable
  int distance = 0;
  int index = 0; // (=> or member variable, enumerator)
  int index_add = 0;
  semantics_checker::LocalVar* lvar_ptr = nullptr;

  //
  //----------------------

  // Kind::MemberVariable
  TypeInfo member_var_type;
  // and location be stored to .ast_class, .index
  // to reference --> { ast_class->member_variables[index] }

  //
  //----------------------

  ASTPtr<Class> ast_class = nullptr;
  ASTPtr<Enum> ast_enum = nullptr;

  TypeInfo self_type; // if member

  semantics_checker::SemaIdentifierEvalResult* S_Evaluated = nullptr;

  static ASTPtr<Identifier> New(Token tok);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  Identifier(Token tok);
};

struct ScopeResol : Named {
  ASTPtr<Identifier> first;
  ASTVec<Identifier> idlist;

  static ASTPtr<ScopeResol> New(ASTPtr<Identifier> first);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  ASTPtr<Identifier> GetLastID() const;

  ScopeResol(ASTPtr<Identifier> first);
};

struct Array : Base {
  ASTVector elements;

  TypeInfo elem_type; // set in Sema

  static ASTPtr<Array> New(Token tok);

  ASTPointer Clone() const override;

  Array(Token tok);
};

struct CallFunc : Base {
  ASTPointer callee; // left side, evaluated to be callable object.
  ASTVector args;

  ASTPtr<Function> callee_ast = nullptr;
  builtins::Function const* callee_builtin = nullptr;

  bool call_functor = false;

  ASTPtr<Enum> ast_enum = nullptr;
  size_t enum_index = 0;

  static ASTPtr<CallFunc> New(ASTPointer callee, ASTVector args = {});

  ASTPointer Clone() const override;

  ASTPtr<Class> get_class_ptr() const;

  CallFunc(ASTPointer callee, ASTVector args = {});
};

struct Expr : Base {
  Token op;
  ASTPointer lhs;
  ASTPointer rhs;

  static ASTPtr<Expr> New(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs);

  ASTPointer Clone() const override;

  Identifier* GetID() override;

  Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs);
};

} // namespace fire::AST