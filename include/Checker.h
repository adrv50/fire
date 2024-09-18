#pragma once

#include "AST.h"

/*
・構文木を探索しているとき、class, enum,
  など型を定義する構文に遭遇したら
  = リストに名前を追加する
    --->
    追加した名前に、付属情報として（どこのスコープに属しているか）を付与。


・型名として評価される構文に遭遇
  = 型名の先頭につける、暗黙的に省略されたスコープ名を調べる。

    AA::BB に自分が位置するとき、C を見つけたとすると、
    BB の中で C を探し、なければ AA 、グローバルの順で探す。
*/

namespace fire::checker {

class Checker {
public:
  struct ScopeInfo {
    ASTPointer ast;
    ASTPtr<AST::Block> block;

    std::string name;

    ScopeInfo(ASTPointer _ast)
        : ast(_ast) {
      switch (ast->kind) {
      case ASTKind::Function: {
        auto x = ast->As<AST::Function>();

        this->block = x->block;
        this->name = x->GetName();

        break;
      }

      case ASTKind::Enum: {
        auto x = ast->As<AST::Enum>();

        this->block = x->enumerators;
        this->name = x->GetName();

        break;
      }

      case ASTKind::Class: {
      }
      }
    }
  };

  struct ScopeLocation {
    std::vector<ScopeInfo> scopes;
    ScopeInfo scope;

    std::string get_name() const {
      std::string ret;

      for (auto&& s : this->scopes)
        ret += s.name + "::";

      return ret + this->scope.name;
    }
  };

  struct IdentifierInfo {
    ASTPointer definition;
    ScopeLocation scope;
  };

  std::vector<IdentifierInfo> IdInfolist;
};

} // namespace fire::checker