#pragma once

#include "AST.h"

namespace fire::checker {

class Checker {
public:
  class NameDuplicationDetector {
  public:
    TokenVector items;

    std::pair<bool, TokenIterator> try_define(Token const& tok) {

      if (auto it = this->find(tok.str); it != items.end())
        return std::make_pair(false, it);

      return std::make_pair(true, this->append(tok));
    }

    TokenIterator find(std::string const& name) {
      for (auto it = this->items.begin(); it != this->items.end();
           it++)
        if (it->str == name)
          return it;

      return this->items.end();
    }

    TokenIterator append(Token const& tok) {
      this->items.emplace_back(tok);
      return this->items.end() - 1;
    }
  };

  Checker(ASTPtr<AST::Program> ast);

  void do_check();

  TypeInfo evaltype(ASTPointer ast);

  void check_statement(ASTPointer ast);

#if !_METRO_DEBUG_
private:
#endif

  struct IdentifierInfo {
    ASTPointer ast;
    Token& tok;

    IdentifierInfo(ASTPointer ast)
        : ast(ast),
          tok(ast->token) {
    }
  };

  struct ScopeContext {
    Checker& C;
    std::vector<IdentifierInfo> ident_list;
    std::vector<ScopeContext> childs;

    IdentifierInfo& append(ASTPointer ast) {
      return this->ident_list.emplace_back(ast);
    }

    ScopeContext& append_child(ScopeContext S) {
      return this->childs.emplace_back(std::move(S));
    }

    ScopeContext(Checker& C)
        : C(C) {
    }
  };

  std::vector<IdentifierInfo>::iterator
  check_and_append(ScopeContext& S, ASTPointer ast);

  ScopeContext make_scope_context(ASTPointer ast);

  ASTPtr<AST::Program> root;

  ScopeContext scope_context;
};

} // namespace fire::checker