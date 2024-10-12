#pragma once

#include "Utils.h"

namespace fire::semantics_checker {

struct LocalVar;
struct ScopeContext;
struct BlockScope;
struct FunctionScope;
struct NamespaceScope;

struct IdentifierInfo;
struct SemaIdentifierEvalResult;

struct SemaContext;

} // namespace fire::semantics_checker

namespace fire::AST {

namespace sema = fire::semantics_checker;

template <class T, class... Args>
ASTPtr<T> ASTNew(Args&&... args) {
#if _DBG_DONT_USE_SMART_PTR_
  return new T(std::forward<Args>(args)...);
#else
  return std::make_shared<T>(std::forward<Args>(args)...);
#endif
}

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

  bool IsConstructedAs(ASTKind k) const {
    return this->_attr.ConstructedAs == k;
  }

  ASTKind GetConstructedKind() const {
    return this->_attr.ConstructedAs;
  }

  bool IsNamed() const {
    return this->_attr.IsNamed;
  }

  bool IsTemplateAST() const {
    return this->_attr.IsTemplate;
  }

  bool is_ident_or_scoperesol() const {
    return this->IsConstructedAs(ASTKind::Identifier)
            || this->IsConstructedAs(ASTKind::ScopeResol);
  }

  bool IsIdentifier() const {
    return this->IsConstructedAs(ASTKind::Identifier);
  }

  bool IsQualifiedIdentifier();
  bool IsUnqualifiedIdentifier();

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

  friend struct sema::LocalVar;
  friend struct sema::ScopeContext;
  friend struct sema::BlockScope;
  friend struct sema::FunctionScope;
  friend struct sema::NamespaceScope;

  struct __attribute__((packed)) AST_Attribute  {
    ASTKind   ConstructedAs;

    bool  IsTemplate;
    bool  IsExpr;
    bool  IsNamed;
  };

  AST_Attribute _attr  = { };

  bool _s_pass_this = false;

  sema::ScopeContext*  ScopeCtxPtr = nullptr;

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

  Vec<ParameterName>  ParameterList;

  //
  // 可変長パラメータ引数
  bool IsVariableParameters = false;
  Token VariableParams_Name;

  size_t ParameterCount() const {
    return ParameterList.size();
  }

  // ASTPtr<Block> owner_block_ptr = nullptr;
  // size_t index_of_self_in_owner_block_list = 0;

  // ASTPointer& InsertInstantiated(ASTPtr<Templatable> ast);

protected:
  Templatable(ASTKind kind, Token tok, Token name)
      : Named(kind, tok, name) {
    this->_attr.IsTemplate = true;
  }

  Templatable(ASTKind k, Token t)
      : Templatable(k, t, t) {
  }

  void _Copy(Templatable const& T) {

    this->ScopeCtxPtr = T.ScopeCtxPtr;

    this->TemplateTok = T.TemplateTok;

    this->IsTemplated = T.IsTemplated;

    this->IsInstantiated = T.IsInstantiated;

    for( auto&& Param : T.ParameterList ) {
      this->ParameterList.emplace_back(Param);
    }

  }
};

} // namespace fire::AST