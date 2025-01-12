#include "Sema/Sema.h"

namespace fire::sema {

ScopeContext* Symbol::get_scope() const {
  return this->parent_table->parent_scope;
}

Symbol::Symbol(SymbolKind kind, SymbolTable* table)
    : kind(kind),
      name(),
      decl(nullptr),
      var(nullptr),
      parent_table(table) {
}

} // namespace fire::sema