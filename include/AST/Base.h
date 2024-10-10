#pragma once

#include "Utils.h"

namespace fire::semantics_checker {

struct ScopeContext;
struct IdentifierInfo;
struct LocalVar;

} // namespace fire::semantics_checker

namespace fire::AST {

struct Base {

  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;
  bool is_expr = false;

  ASTKind _constructed_as;

  bool _s_pass_this = false;

  bool IsTemplateAST() const {
    return this->_is_template_ast;
  }

  bool is_ident_or_scoperesol() const {
    return this->_constructed_as == ASTKind::Identifier ||
           this->_constructed_as == ASTKind::ScopeResol;
  }

  template <class T>
  T* As() {
    return static_cast<T*>(this);
  }

  template <class T>
  T const* As() const {
    return static_cast<T const*>(this);
  }

  Expr const* as_expr() const;
  Statement const* as_stmt() const;

  Expr* as_expr();
  Statement* as_stmt();

  Value const* as_value() const;

  bool is(ASTKind k);
  bool is_qual_id();
  bool is_id_nonqual();

  virtual Identifier* GetID();

  virtual ASTPointer Clone() const = 0;

  [[maybe_unused]] i64 GetChilds(ASTVector& out) const;

  virtual ~Base() = default;

  semantics_checker::ScopeContext* scope_ctx_ptr = nullptr;

protected:
  Base(ASTKind kind, Token token);
  Base(ASTKind kind, Token token, Token endtok);

  bool _is_template_ast = false;
};

struct Named : Base {
  Token name;

  string const& GetName() const;

protected:
  Named(ASTKind kind, Token tok, Token name);

  Named(ASTKind kind, Token tok);
};

struct Templatable : Named {
  Token tok_template;

  bool is_templated = false;

  bool is_instantiated = false;

  struct ParameterName {
    Token token;
    Vec<ParameterName> params;

    string to_string() const {
      string s = token.str;

      if (params.size() >= 1) {
        s += "<" +
             utils::join(", ", params,
                         [](auto const& P) -> string {
                           return P.to_string();
                         }) +
             ">";
      }

      return s;
    }
  };

  Vec<ParameterName> template_param_names;

  //
  // 可変長パラメータ引数
  bool IsVariableParameters = false;
  Token VariableParams_Name;

  size_t ParameterCount() const {
    return template_param_names.size();
  }

  ASTPtr<Block> owner_block_ptr = nullptr;
  size_t index_of_self_in_owner_block_list = 0;

  ASTPointer& InsertInstantiated(ASTPtr<Templatable> ast);

protected:
  Templatable(ASTKind kind, Token tok, Token name)
      : Named(kind, tok, name) {
    this->_is_template_ast = true;
  }

  Templatable(ASTKind k, Token t)
      : Templatable(k, t, t) {
  }

  void _Copy(Templatable const* _t);
};

} // namespace fire::AST