#pragma once

#include "AST.h"

namespace fire::parser {

class Parser {

public:
  Parser(TokenVector& tokens);

  // ASTPointer Ident();

  ASTPointer Factor();
  ASTPointer ScopeResol();

  ASTPointer Lambda();

  ASTPointer IndexRef();
  ASTPointer Unary();

  ASTPointer Mul();
  ASTPointer Add();
  ASTPointer Shift();
  ASTPointer Compare();
  ASTPointer BitCalc();
  ASTPointer LogAndOr();
  ASTPointer Assign();

  ASTPointer Expr();
  ASTPointer Stmt();

  ASTPointer Top();

  ASTPtr<AST::Block> Parse();

private:
  bool check() const;

  bool eat(std::string_view str);
  void expect(std::string_view str, bool keep_token = false);

  // ----
  // for brackets of template parameter "<" ">"
  //
  // 閉じ括弧で右シフト演算子がある場合，自動的に分割します．
  // フラグの都合のため eat(), expect() ではなく以下の関数群を使うこと
  //
  bool eat_typeparam_bracket_open();     //  eat "<"
  bool eat_typeparam_bracket_close();    //  eat ">"
  void expect_typeparam_bracket_open();  // expect "<"
  void expect_typeparam_bracket_close(); // expect ">"
  // -----

  bool match(std::string_view s) {
    return this->cur->str == s;
  }

  bool match(TokenKind kind) {
    return this->cur->kind == kind;
  }

  bool match(std::pair<TokenKind, std::string_view> pair) {
    return this->match(pair.first) && this->match(pair.second);
  }

  template <class T, class U, class... Args>
  bool match(T&& t, U&& u, Args&&... args) {
    if (!this->check() || !this->match(std::forward<T>(t))) {
      return false;
    }

    auto save = this->cur;
    this->cur++;

    auto ret = this->match(std::forward<U>(u), std::forward<Args>(args)...);

    this->cur = save;
    return ret;
  }

  TokenIterator insert_token(Token tok) {
    tok.sourceloc = this->cur->sourceloc;

    this->cur = this->tokens.insert(this->cur, tok);
    this->end = this->tokens.end();

    return this->cur;
  }

  TokenIterator expectIdentifier();

  ASTPtr<AST::TypeName> expectTypeName();

  ASTPtr<AST::Signature> expect_signature();

  static ASTPtr<AST::Expr> new_expr(ASTKind k, Token& op, ASTPointer lhs,
                                    ASTPointer rhs) {
    return AST::Expr::New(k, op, lhs, rhs);
  }

  static ASTPtr<AST::Expr> new_assign(ASTKind kind, Token& op, ASTPointer lhs,
                                      ASTPointer rhs) {
    return new_expr(ASTKind::Assign, op, lhs, new_expr(kind, op, lhs, rhs));
  }

  TokenVector& tokens;
  TokenIterator cur, end, ate;

  bool _in_class = false;
  ASTPtr<AST::Class> _classptr = nullptr;

  bool _in_loop = false;

  int _typeparam_bracket_depth = 0;

  //
  // fn func <...>
  // class C <...>
  AST::Templatable::ParameterName parse_template_param_decl() {
    AST::Templatable::ParameterName pn = {.token = *this->expectIdentifier(),
                                          .params = {}};

    if (this->eat_typeparam_bracket_open()) {
      do {
        pn.params.emplace_back(parse_template_param_decl());
      } while (this->eat(","));

      this->expect_typeparam_bracket_close();
    }

    return pn;
  }

  void check_match_of_template_arg_type(AST::Templatable::ParameterName const& P,
                                        ASTPtr<AST::TypeName> T) {

    if (P.token.str != T->GetName())
      return;
  }
};

} // namespace fire::parser