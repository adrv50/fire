#pragma once

#include <functional>

#include "Node/Node.h"
#include "Symbol.h"

namespace fire::sema {

struct ScopeContext;

struct SymbolTable {

  ScopeContext* parent_scope;

  Vec<Symbol*> symbols;

  typename Vec<Symbol*>::iterator begin();
  typename Vec<Symbol*>::iterator end();

  Symbol*& operator[](size_t index);

  Symbol*& push(Symbol* sym);
  void pop();

  size_t find(Vec<Symbol*>& out, string const& name);
  size_t find_if(Vec<Symbol*>& out, std::function<bool(Symbol*)> pred);

  Vec<Symbol*> get_if(std::function<bool(Symbol*)> pred);

  SymbolTable(ScopeContext* parent_scope);
};

} // namespace fire::sema