#include "Sema/Sema.h"

namespace fire::sema {

ScopeContext* Symbol::get_parent_scope() const {
  return this->parent_table ? this->parent_table->parent_scope : nullptr;
}

string Symbol::get_full_scoped_name() const {
  string ret = this->name;

  if (ScopeContext* scope = this->get_parent_scope())
    do {
      if (string tmp; scope->get_name(tmp))
        ret.insert(0, tmp + "::");
    } while ((scope = scope->parent));

  return ret;
}

Symbol::Symbol(SymbolKind kind, SymbolTable* table)
    : kind(kind),
      name(),
      decl(nullptr),
      var(nullptr),
      parent_table(table) {
}

} // namespace fire::sema