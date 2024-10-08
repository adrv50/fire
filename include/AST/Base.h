#pragma once

namespace fire::AST {

struct Base {
  ASTKind kind;
  Token token;
  Token endtok;

  bool is_named = false;
  bool is_expr = false;

  ASTKind _constructed_as;

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

protected:
  Base(ASTKind kind, Token token);
  Base(ASTKind kind, Token token, Token endtok);
};

struct Named : Base {
  Token name;

  string_view const& GetName() const;

protected:
  Named(ASTKind kind, Token tok, Token name);

  Named(ASTKind kind, Token tok);
};

struct Templatable : Named {
  Token tok_template;

  bool is_templated = false;

  TokenVector template_param_names;

protected:
  using Named::Named;

  void _Copy(Templatable const* _t);
};

} // namespace fire::AST