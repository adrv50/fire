#pragma once

#include "ScopeContext.h"

namespace fire::sema {

struct FunctionContext {
  Vec<Node*> return_stmt_list;

  Vec<Symbol*> let_stmt_sym_ptr_list;
};

struct NodeContext {
  ScopeContext* scope = nullptr;

  FunctionContext* func = nullptr;

  Symbol* let_sym_ptr = nullptr;
};

} // namespace fire::sema