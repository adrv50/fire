#pragma once

#include "Node.h"
#include "Token.h"
#include "TypeInfo.h"

#include "../Error.h"

#include "ScopeContext.h"
#include "NodeContext.h"

namespace fire::sema {

enum ExprEvalStatus {
  EX_Succeed,

  EX_Error,
};

struct ExprEvalContext {
  bool in_call_func = false;
  Node* callfunc_nd = nullptr;
  Vec<TypeInfo>* callfunc_args_p = nullptr;

  bool allowed_empty_array = false;
  Node* let_stmt = nullptr;
};

class Sema;
class ExprEval {
  Sema& S;

  ExprEvalContext ctx;

  Vec<ExprEvalContext> _saves;

  void save();
  void restore();

public:
  ExprEval(Sema& S);

  TypeInfo eval(Node* node);

  TypeInfo make_type_from_symbol(Symbol* sym);

  TypeInfo operator()(Node* node) {
    return this->eval(node);
  }
};

struct FunctionContext {
  Vec<Node*> return_stmt_list;
};

class Sema {
  friend struct Symbol;
  friend struct SymbolTable;
  friend struct ScopeContext;

  friend class ExprEval;

  Node* program;

  ScopeContext* root_scope;

  ScopeContext* cur_scope;

  ExprEval expr_eval;

  ScopeContext* enter_scope(ScopeContext* scope);
  void leave_scope();

public:
  Sema(Node* program);

  Sema(Sema&&) = delete;
  Sema(Sema const&) = delete;

  void check_all();

  void check_func(Node* node);

  void check_stmt(Node* node);

  TypeInfo eval_type_ti(Node* node);

private:
  //
  // find_name:
  //   find in scope chain (current to root)
  size_t find_name(Vec<Symbol*> out, string const& name, ScopeContext* start = nullptr);
};

} // namespace fire::sema