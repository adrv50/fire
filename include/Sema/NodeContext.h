#pragma once

#include "ScopeContext.h"

namespace fire::sema {

struct FunctionContext {
  templates::DefinitionIR* template_ir = nullptr;

  Vec<Node*> return_stmt_list;

  Vec<Symbol*> let_stmt_sym_ptr_list; // => for count of all local variables
};

struct NodeContext {
  ScopeContext* scope = nullptr;

  FunctionContext* func = nullptr;

  Symbol* let_sym_ptr = nullptr;
};

} // namespace fire::sema