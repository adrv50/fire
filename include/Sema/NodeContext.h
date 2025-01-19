#pragma once

#include "ScopeContext.h"

namespace fire::sema {

struct FunctionContext {
  templates::DefinitionIR* template_ir = nullptr;

  Vec<Node*> return_stmt_list;

  Vec<Symbol*> let_stmt_sym_ptr_list; // => for count of all local variables

  size_t get_lvar_count() const {
    return this->let_stmt_sym_ptr_list.size();
  }
};

struct NodeContext {
  ScopeContext* scope = nullptr;

  FunctionContext* func = nullptr;

  Symbol* let_sym_ptr = nullptr;
};

} // namespace fire::sema