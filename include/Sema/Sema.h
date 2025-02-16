#pragma once

#include <functional>

#include "Node/Node.h"
#include "Token/Token.h"

#include "ScopeContext.h"
#include "NodeContext.h"

#include "ExprEval.h"
#include "Templates.h"

namespace fire::sema {
class Sema {
  friend struct Symbol;
  friend struct SymbolTable;
  friend struct ScopeContext;

  friend class ExprEval;
  friend struct ExprEvalResult;

  Node* program;

  ScopeContext* root_scope;

  ScopeContext* cur_scope;

  ExprEval expr_eval;

  templates::TemplateManager tp_manager;

  ScopeContext* enter_scope(ScopeContext* scope);
  void leave_scope();

  ScopeContext* find_scope(std::function<bool(ScopeContext*)> pred);

  ScopeContext* get_cur_func_scope();
  ScopeContext* get_cur_class_scope();

public:
  Sema(Node* program);

  Sema(Sema&&) = delete;
  Sema(Sema const&) = delete;

  void check_all();

  void check_top_item(Node* node);

  void check_class(Node* node);

  void check_func(Node* node, Node* parent_class = nullptr);

  void check_let(Node* node, Node* parent_class = nullptr);

  void check_stmt(Node* node);

  TypeInfo eval_type_ti(Node* node);

  void handle_type_kind_error(Symbol* sym, Node* nd, const Vec<TypeInfo>& tp_args,
                              const string& name);

  static Sema* get_instance();

private:
  //
  // find_name:
  //   find in scope chain (current to root)
  size_t find_name(Vec<Symbol*>& out, string const& name, ScopeContext* start = nullptr,
                   bool once = false);

  size_t global_or_namespace_var_offset = 0;

  static Sema* _instance;
};

} // namespace fire::sema