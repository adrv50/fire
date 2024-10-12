#pragma once

#include "Utils.h"

namespace fire::semantics_checker {

struct LocalVar;
struct ScopeContext;

struct IdentifierInfo;
struct SemaIdentifierEvalResult;

struct SemaContext;

} // namespace fire::semantics_checker

namespace fire::AST {

using sema = semantics_checker;

struct Base {

  friend class Sema;

  ASTKind kind;

  Token token;
  Token endtok;

  // bool is_named = false;
  // bool is_expr = false;

  bool Is(ASTKind k) const {
    return this->kind == k;
  }

  bool IsExpr() const {
    return this->_attr.IsExpr;
  }

  bool IsConstructedAs(ASTkind k) const {
    return this->_attr.ConstructedAs == k;
  }

  bool IsNamed() const {
    return this->_attr.IsNamed;
  }

  bool IsTemplateAST() const {
    return this->_attr.IsTemplate;
  }

  bool is_ident_or_scoperesol() const {
    return this->IsConstrucedAs(ASTKind::Identifier)
            || this->IsConstructedAs(ASTKind::ScopeResol);
  }

  bool IsIdentifier() const {
    return this->IsConstrucedAs(ASTKind::Identifier);
  }

  bool IsQualifiedIdentifier() const;

  bool IsUnqualifiedIdentifier() const;

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

  virtual Identifier* GetID();

  virtual ASTPointer Clone() const = 0;

  [[maybe_unused]]
  i64 GetChilds(ASTVector& out) const;

  virtual ~Base() = default;

protected:

  __attribute__((packed))
  struct AST_Attribute {
    ASTKind   ConstructedAs;

    bool  IsTemplate;
    bool  IsExpr;
    bool  IsNamed;
  };

  AST_Attribute _attr  = { };

  bool _s_pass_this = false;

  sema::ScopeContext* scope_ctx_ptr = nullptr;

  Base(ASTKind k, Token token, Token endtok)
    : kind(k),
      token(std::move(token)),
      endtok(std::move(endtok))
  {
    this->_attr.ConstructedAs = k;
  }

  Base(ASTKind kind, Token token)
    : Base(kind, token, token)
  {
  }

};

struct Named : Base {
  Token name;

  string const& GetName() const {
    return this->name.str;
  }

protected:
  Named(ASTKind k, Token tok, Token name)
    : Base(k, tok),
      name(name)
  {
    this->_attr.IsNamed = true;
  }

  Named(ASTKind k, Token t)
    : Named(k, t, t)
  {
  }
};

struct Templatable : Named {

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

  Token TemplateTok;  // "<"

  bool IsTemplated = false;

  bool IsInstantiated = false;

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