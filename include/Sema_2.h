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

struct SemaFunctionContext {
  Node* func;

  Vec<VarInfo*> pvar_list;

  Vec<pair<Node*, TypeInfo>> return_stmt_list;

  Node* result_type_nd = nullptr;
  TypeInfo result_type;

  SemaFunctionContext(Node* func)
      : func(func),
        pvar_list(),
        return_stmt_list() {
  }
};

enum SymbolKind : u8 {
  SY_Unknown,

  SY_Var,
  SY_Func,

  SY_Enum,
  SY_Struct,
  SY_Class,

  SY_Namespace,
};

struct Symbol {

  SymbolKind kind = SY_Unknown;

  string name;

  Node* decl = nullptr;

  Symbol() {
  }
};

struct ScopeContext;
struct SymbolTable {

  ScopeContext* parent_scope;
};

struct ScopeContext {

  Node* node;
  ScopeType type;

  ScopeContext* parent = nullptr;
  Vec<ScopeContext*> childs;

  VarList variables;

  SemaFunctionContext* func_ctx = nullptr;

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

          scope->variables.append(item->nd_let_name->str).decl = item;
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
      scope->variables.append(arg->nd_func_arg_name->str).is_type_deducted = true;
    }

    scope->append(ScopeContext::from_block(node->nd_func_body));

    scope->func_ctx = new SemaFunctionContext(node);

    scope->func_ctx->result_type_nd = node->nd_func_result_type;

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

  Vec<IdentifierInfo> tp_args;

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
        tp_args() {
  }
};

struct IDEvalResult {
  IdentifierInfo info;
  TypeInfo type;

  IDEvalResult(IdentifierInfo ii, TypeInfo type)
      : info(std::move(ii)),
        type(std::move(type)) {
  }
};

struct ExprEvalContext {
  bool in_call_func = false;
  Node* call_func_expr = nullptr;
  Vec<TypeInfo>* call_args_ptr = nullptr;

  bool as_functor = false;
  bool as_initializer = false;

  //
  // 空の配列を評価する際、同じ文脈に型の定義が
  // ある場合は、評価された型情報へのポインタを指す。
  // ( 例: "let a : vector<int> = [ ];" など )
  TypeInfo* container_elem_type_p = nullptr;
  Node* container_elem_type_def_nd = nullptr;

  bool in_array = false;
  bool in_tuple = false;

  //
  // is directly linked to a statement
  // (almost condition)
  bool in_statement = false;
  Node* stmt_nd = nullptr;
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

  ScopeContext* get_cur_func_scope() {
    return this->search_scope_if([](ScopeContext* S) {
      return S->type == SC_Function;
    });
  }

  ScopeContext* get_cur_loop_scope() {
    return this->search_scope_if([](ScopeContext* S) {
      return S->type == SC_Block && S->node->nd_block_parent->is_loop_stmt();
    });
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

  static bool get_same_name_in_scope(ScopeContext* scope, string const& name,
                                     IdentifierInfo& II) {
    auto pvar = scope->variables.find(name);

    if (pvar) {
      II.kind = ID_Variable;
      II.scope = scope;
      II.pvar = pvar;

      return true;
    }

    if (scope->find([&](ScopeContext* C) -> bool {
          if (C->type == SC_Function && C->node->nd_func_name->str == name) {
            II.kind = ID_Function;
            II.scope = C;
            II.func_candidates.emplace_back(C->node);
          }

          return false;
        }))
      return true;

    return false;
  }

  static void
  limit_builtin_func_candidates(Vec<Builtins::BuiltinFunc const*>& candidates,
                                std::function<bool(Builtins::BuiltinFunc const*)> pred) {
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(), pred),
                     candidates.end());
  }

  IdentifierInfo get_id_info(Node* id, ScopeContext* find_in = nullptr,
                             bool to_reverse = true,
                             TypeKind* find_in_builtin_type = nullptr) {

    Vec<Node*> sr;

    if (id->is(ND_ScopeResol)) {
      sr = id->nd_scope_resol_idlist;
      id = id->nd_scope_resol_first;
    }

    string const& name = id->nd_id_name->str;

    IdentifierInfo II{id, name};

    if (find_in_builtin_type) {
      // => find method in type

      auto& tk = *find_in_builtin_type;

      if (Builtins::BuiltinFunc::find(II.builtin_func_candidates, name) >= 1) {
        limit_builtin_func_candidates(II.builtin_func_candidates,
                                      [&tk](Builtins::BuiltinFunc const* bfunc) -> bool {
                                        return !bfunc->is_method ||
                                               (bfunc->self_type.kind != tk);
                                      });

        if (II.builtin_func_candidates.size() >= 1)
          II.kind = ID_BuiltinFunc;
        else
          Error(id, "cannot find built-in method '" + name + "' in type '" +
                        TypeInfo::get_name_of_kind(tk) + "'")
              .crash();
      }
    }
    else {
      if (!find_in)
        find_in = this->CurScope;

      this->search_scope_if(std::bind(Sema::get_same_name_in_scope, std::placeholders::_1,
                                      name, std::reference_wrapper(II)),
                            find_in, to_reverse);
    }

    if (II.kind == ID_Variable && !II.pvar->is_type_deducted)
      Error(id, "cannot use variable before type deduction").crash();

    if (II.kind == ID_Unknown)
      this->find_builtin_name(II);

    if (II.kind == ID_Unknown) {
      II.typekind = TypeInfo::get_kind_of_name(name);

      if (II.typekind != TypeKind::Unknown)
        II.kind = ID_BuiltinType;
    }

    if (II.kind == ID_Unknown)
      Error(id, "use of undefined name '" + name + "'").crash();

    if (!id->nd_id_tp_args.empty()) {
      switch (II.kind) {
        case ID_Variable:
          Error(id->tok->next, "variable '" + name + "' is not template.").crash();

        case ID_Namespace:
          Error(id->tok->next, "namespace '" + name + "' is not template.").crash();
      }
    }

    for (auto&& t_arg : id->nd_id_tp_args) {
      II.tp_args.emplace_back(this->get_id_info(t_arg, find_in, to_reverse));
    }

    Token* prev = id->first_tok;

    for (auto&& sub : sr) {
      auto const& name = sub->nd_id_name->str;
      auto op = sub->first_tok->prev;

      switch (II.kind) {
        case ID_Variable:
        case ID_Function:
        case ID_BuiltinFunc:
        case ID_Enumerator:
          Error(op, "invalid use of scope resolution operator").crash();

        case ID_BuiltinType: {
          II = this->get_id_info(sub, nullptr, false, &II.typekind);
          break;
        }

        case ID_Enum:
          todo_impl;

        case ID_Struct:
          todo_impl;

        case ID_Class:
          todo_impl;

        case ID_Namespace:
          todo_impl;
      }

      for (auto&& t_arg : sub->nd_id_tp_args) {
        II.tp_args.emplace_back(
            this->get_id_info(t_arg, find_in, to_reverse, find_in_builtin_type));
      }

      prev = sub->first_tok;
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

  TypeInfo get_type_of_id_info(ExprEvalContext ctx, IdentifierInfo const& II) {
    switch (II.kind) {
      case ID_Variable:
        return II.pvar->type;

      case ID_Function: {
        auto candidates = II.func_candidates;

        if (candidates.size() >= 2) {
          if (ctx.in_call_func) {
            todo_impl; // limit candidates
          }
          else {
            Error e{II.id, "ambiguous function name '" + II.name + "'"};

            for (auto&& cd : candidates)
              e.add_note(cd->tok, "candidate:");

            e.crash();
          }
        }

        assert(candidates.size() == 1);

        auto func = candidates[0];

        if (func->nd_func_is_template) {
          todo_impl;
        }

        return Sema::make_functor_type(func);
      }

      case ID_BuiltinFunc: {
        auto candidates = II.builtin_func_candidates;

        if (candidates.size() >= 2) {
          if (ctx.in_call_func) {
            todo_impl; // limit candidates
          }
          else {
            Error e{II.id, "ambiguous function name '" + II.name + "'"};

            for (auto&& cd : candidates)
              e.add_note("candidate: " + cd->to_string());

            e.crash();
          }
        }

        assert(candidates.size() == 1);

        auto bfun = candidates[0];

        if (bfun->is_template) {
          todo_impl;
        }

        return Sema::make_functor_type(bfun);
      }
    }

    todo_impl;
  }

  IDEvalResult eval_id(ExprEvalContext ctx, Node* id, ScopeContext* scope = nullptr,
                       bool find_reverse = true, bool expected_as_type_name = false,
                       bool in_call_func_expr = false) {
    if (!scope)
      scope = this->CurScope;

    auto ii = this->get_id_info(id, scope);

    if (expected_as_type_name && !ii.is_type_name()) {
      Error(id, "expected type name").crash();
    }

    return {ii, this->get_type_of_id_info(ctx, ii)};
  }

  bool resolv_template_parameter_types() {

    todo_impl;
  }

  TypeInfo make_functor_type(Node* func) {
    TypeInfo ti = TypeKind::Functor;

    for (auto&& arg : func->nd_func_args)
      ti.tp_args.emplace_back(this->eval_type_ti(arg->nd_func_arg_type));

    ti.tp_args.insert(ti.tp_args.begin(), this->eval_type_ti(func->nd_func_result_type));

    return ti;
  }

  TypeInfo make_functor_type(Builtins::BuiltinFunc const* bfun) {
    TypeInfo ti = TypeKind::Functor;

    ti.tp_args = bfun->arg_types;
    ti.tp_args.insert(ti.tp_args.begin(), bfun->ret_type);

    return ti.set_ftor_bfun(bfun);
  }

  TypeInfo expect_type(ExprEvalContext ctx, Node* node, TypeInfo const& exp) {
    auto ti = this->eval_expr_ti(node, ctx);

    if (!ti.equals(exp))
      Error(node, "expected '" + exp.to_string() + "' type expression, but found '" +
                      ti.to_string() + "'")
          .crash();

    return ti;
  }

  enum ArgumentMatchings {
    AM_None,

    //
    // ready to call
    AM_Ok,

    //
    // tried to call not callable object
    AM_NotCallable,

    //
    // too many.
    AM_TooMany,

    //
    // too few.
    AM_TooFew,

    //
    // type mismatch.
    AM_TypeMismatch,
  };

  struct CallFuncExprCheckResult {
    Node* call;

    ArgumentMatchings arg_match;

    TypeInfo result_type;

    Error* err;

    CallFuncExprCheckResult()
        : call(nullptr),
          arg_match(AM_None),
          result_type(),
          err(nullptr) {
    }
  };

  CallFuncExprCheckResult check_call_func_expr(ExprEvalContext ctx, Node* node) {
    auto keep = ctx;

    Vec<TypeInfo> arg_types;

    for (auto&& arg : node->nd_callfunc_args)
      arg_types.emplace_back(this->eval_expr_ti(arg, ctx));

    ctx.in_call_func = true;
    ctx.call_func_expr = node;
    ctx.call_args_ptr = &arg_types;
    ctx.as_functor = true;

    auto functor_ti = this->eval_expr_ti(node->nd_callfunc_callee, ctx);

    if (!functor_ti.is_callable())
      Error(node->nd_callfunc_callee,
            "'" + functor_ti.to_string() + "' type object is not callable")
          .crash();

    if (functor_ti.ftor_node)
      node->nd_callfunc_callee_userdef = functor_ti.ftor_node;
    else
      node->nd_callfunc_callee_builtin = functor_ti.ftor_blt;

    ctx = keep;

    CallFuncExprCheckResult check_result;

    check_result.call = node;

    check_result.result_type = functor_ti.tp_args[0];

    return check_result;
  }

public:
  Sema(Node* program);

  void check_full();

  void check_func(Node* node);

  void check_block(Node* node);

  void check_stmt(Node* node);

  void check_let(Node* node);

  TypeInfo eval_expr_ti(Node* node, ExprEvalContext ctx);

  TypeInfo eval_type_ti(Node* node);
};

} // namespace sema