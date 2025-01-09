#pragma once

#include <concepts>
#include <algorithm>
#include <cassert>

#include "Token.h"
#include "Node.h"
#include "TypeInfo.h"
#include "Object.h"
#include "Builtins.h"

#include "Utils.h"
#include "Error.h"
#include "alert.h"

namespace sema {

enum ScopeType {
  SC_Block,

  SC_Function,

  SC_Enum,
  SC_Class,
  SC_Struct,

  SC_Namespace,
};

struct VarInfo {
  string name;
  TypeInfo type;
  Node* decl;
  size_t offset;
  bool is_type_deducted;

  VarInfo(string const& name, TypeInfo type = {})
      : name(name),
        type(type),
        decl(nullptr),
        offset(0),
        is_type_deducted(false) {
  }

  VarInfo()
      : VarInfo("", {}) {
  }
};

struct VarList {
  Vec<VarInfo> list;

  auto begin() {
    return this->list.begin();
  }

  auto end() {
    return this->list.end();
  }

  template <typename... Args>
  VarInfo& append(Args&&... args) {
    return this->list.emplace_back(std::forward<Args>(args)...);
  }

  VarInfo& operator[](size_t index) {
    return this->list[index];
  }

  size_t size() const {
    return this->list.size();
  }

  VarInfo* find(string const& name) {
    for (auto&& var : this->list)
      if (var.name == name)
        return &var;

    return nullptr;
  }
};

struct ScopeContext {

  Node* node;
  ScopeType type;

  ScopeContext* parent = nullptr;
  Vec<ScopeContext*> childs;

  VarList variables;

  ScopeContext* find(std::function<bool(ScopeContext*)> pred, bool recursive = false) {
    for (auto&& C : this->childs) {
      if (pred(C))
        return C;

      if (recursive)
        if (auto cc = C->find(pred, true))
          return cc;
    }

    return nullptr;
  }

  ScopeContext* find(ScopeContext* child) {
    for (auto&& c : this->childs)
      if (c == child)
        return c;

    return nullptr;
  }

  template <typename... Args>
  requires std::constructible_from<ScopeContext, Args...> ScopeContext*
  append(Args&&... args) {
    auto child = this->childs.emplace_back(std::forward<Args>(args)...);

    child->parent = this;

    return child;
  }

  ScopeContext* append(ScopeContext* scope) {
    auto child = this->childs.emplace_back(scope);

    child->parent = this;

    return child;
  }

  static ScopeContext* from_block(Node* node) {
    auto scope = new ScopeContext(node, SC_Block);

    for (auto&& item : node->nd_items) {
      switch (item->kind) {
        case ND_Let:
          item->sema_scope = scope;
          item->nd_let_offset = scope->variables.size();
          scope->variables.append(item->nd_let_name->str);
          break;

        case ND_Block:
          scope->append(ScopeContext::from_block(item));
          break;

        case ND_Function:
          scope->append(ScopeContext::from_func(item));
          break;
      }
    }

    return scope;
  }

  static ScopeContext* from_func(Node* node) {

    auto scope = new ScopeContext(node, SC_Function);

    for (auto&& arg : node->nd_func_args) {
      scope->variables.append(arg->nd_func_arg_name->str);
    }

    scope->append(ScopeContext::from_block(node->nd_func_body));

    return scope;
  }

  ScopeContext(Node* node, ScopeType type)
      : node(node),
        type(type),
        childs() {
    node->sema_scope = this;
  }
};

enum IdentifierKind {
  ID_Unknown,
  ID_Variable,
  ID_Function,

  ID_Enum,
  ID_Enumerator,
  ID_Struct,
  ID_Class,
  ID_Namespace,

  ID_BuiltinType,

  ID_BuiltinFunc,
};

struct IdentifierInfo {
  Node* id; // ND_Identifier
  string name;

  IdentifierKind kind;

  ScopeContext* scope; // The ScopeContext contains target

  Vec<IdentifierInfo> template_args;

  TypeKind typekind = TypeKind::Unknown; // => ID_BuiltinType

  VarInfo* pvar = nullptr;

  Vec<Node*> func_candidates;
  Vec<Builtins::BuiltinFunc const*> builtin_func_candidates;

  bool is_type_name() const {
    switch (this->kind) {
      case ID_Enum:
      case ID_Struct:
      case ID_Class:
        return true;

      default:
        return false;
    }
  }

  IdentifierInfo(Node* id = nullptr, string const& name = "")
      : id(id),
        name(name),
        kind(ID_Unknown),
        scope(nullptr),
        template_args() {
  }
};

struct FunctionSignature {
  TypeInfo result_type;
  Vec<TypeInfo> arg_types;

  Node* node = nullptr;
};

class Sema {

  Node* Program = nullptr;

  ScopeContext* RootScope = nullptr;

  ScopeContext* CurScope = nullptr;

  bool is_in_func() {
    return this->get_cur_func_scope() != nullptr;
  }

  void enter_scope(ScopeContext* scope) {
    this->CurScope = this->CurScope->find(scope);

    assert(this->CurScope);
  }

  void enter_scope(Node* node) {
    this->enter_scope(node->sema_scope);
  }

  void leave_scope() {
    this->CurScope = this->CurScope->parent;
  }

  //
  // search to reverse (-> parent)
  ScopeContext* search_scope_if(std::function<bool(ScopeContext*)> pred,
                                ScopeContext* begin = nullptr, bool to_reverse = true) {
    if (!begin)
      begin = this->CurScope;

    if (!to_reverse)
      return pred(begin) ? begin : nullptr;

    while (begin && !pred(begin))
      begin = begin->parent;

    return begin;
  }

  ScopeContext* get_cur_func_scope() {
    return this->search_scope_if([](ScopeContext* S) {
      return S->type == SC_Function;
    });
  }

  IdentifierInfo get_id_info(Node* id, ScopeContext* find_in = nullptr,
                             bool to_reverse = true) {
    assert(id->is(ND_Identifier));

    string const& name = id->nd_id_name->str;

    IdentifierInfo II{id, name};

    if (!find_in)
      find_in = this->CurScope;

    ScopeContext* scope = this->search_scope_if(
        [&](ScopeContext* S) -> bool {
          auto pvar = S->variables.find(name);

          if (pvar) {
            II.kind = ID_Variable;
            II.scope = S;

            II.pvar = pvar;

            return true;
          }

          if (S->find([&](ScopeContext* C) -> bool {
                if (C->type == SC_Function && C->node->nd_func_name->str == name) {
                  II.kind = ID_Function;
                  II.scope = C;
                  II.func_candidates.emplace_back(C->node);
                }

                return false;
              }))
            return true;

          return false;
        },
        find_in, to_reverse);

    if (II.kind == ID_Unknown)
      this->find_builtin_name(II);

    if (II.kind == ID_Unknown) {
      II.typekind = TypeInfo::get_kind_of_name(name);

      if (II.typekind != TypeKind::Unknown)
        II.kind = ID_BuiltinType;
    }

    for (auto&& t_arg : id->nd_id_template_args) {
      II.template_args.emplace_back(this->get_id_info(t_arg, find_in, to_reverse));
    }

    return II;
  }

  void find_builtin_name(IdentifierInfo& II) {

    if (auto count = Builtins::BuiltinFunc::find(II.builtin_func_candidates, II.name);
        count >= 1) {
      II.kind = ID_BuiltinFunc;
    }
  }

  FunctionSignature make_func_signature(Node* func) {
    FunctionSignature sig;

    sig.node = func;
    sig.result_type = this->eval_type_ti(func->nd_func_result_type);

    for (auto&& arg : func->nd_func_args)
      sig.arg_types.emplace_back(this->eval_type_ti(arg->nd_func_arg_type));

    return sig;
  }

  static VarInfo& get_varinfo_from_let_nd(Node* node) {
    return node->sema_scope->variables[node->nd_let_offset];
  }

  TypeInfo eval_id(Node* id, ScopeContext* scope = nullptr, bool find_reverse = true) {
    if (!scope)
      scope = this->CurScope;

    IdentifierInfo ii;

    if (id->is(ND_Identifier))
      ii = this->get_id_info(id);
    else if (id->is(ND_ScopeResol)) {
      ii = this->get_id_info(id->nd_scope_resol_first);

      for (auto&& sub : id->nd_scope_resol_idlist) {
        switch (ii.kind) {
          case ID_Variable:
          case ID_Function:
            Error(sub->first_tok->prev, "invalid use of scope-resolution operator")
                .crash();

          case ID_Class: {
            todo_impl;
          }

          case ID_BuiltinType: {
          }
        }
      }
    }

    switch (ii.kind) {
      case ID_Variable: {
        if (!ii.pvar->is_type_deducted)
          Error(id, "cannot use variable before type deduction").crash();

        return ii.pvar->type;
      }

      case ID_Function: {
        todo_impl;
      }

      case ID_BuiltinType: {
        TypeInfo type =
      }

      case ID_BuiltinFunc: {
        alertmsg(ii.builtin_func_candidates.size());

        todo_impl;
      }
    }

    Error(id, "undefined name").crash();
  }

public:
  Sema(Node* program);

  void check_full();

  void check_func(Node* node);

  void check_block(Node* node);

  void check_stmt(Node* node);

  void check_let(Node* node);

  TypeInfo eval_expr_ti(Node* node);

  TypeInfo eval_type_ti(Node* node);
};

} // namespace sema