#include "Sema/SymbolTable.h"

namespace fire::sema {

typename Vec<Symbol*>::iterator SymbolTable::begin() {
  return this->symbols.begin();
}

typename Vec<Symbol*>::iterator SymbolTable::end() {
  return this->symbols.end();
}

Symbol*& SymbolTable::push(Symbol* sym) {
  return this->symbols.emplace_back(sym);
}

void SymbolTable::pop() {
  this->symbols.pop_back();
}

Symbol*& SymbolTable::operator[](size_t index) {
  return this->symbols[index];
}

size_t SymbolTable::find(Vec<Symbol*>& out, string const& name) {
  for (auto&& sym : this->symbols)
    if (sym->name == name)
      out.emplace_back(sym);

  return out.size();
}

size_t SymbolTable::find_if(Vec<Symbol*>& out, std::function<bool(Symbol*)> pred) {
  for (auto&& sym : this->symbols)
    if (pred(sym))
      out.emplace_back(sym);

  return out.size();
}

Vec<Symbol*> SymbolTable::get_if(std::function<bool(Symbol*)> pred) {
  Vec<Symbol*> v;

  for (auto&& sym : this->symbols)
    if (pred(sym))
      v.push_back(sym);

  return v;
}

SymbolTable::SymbolTable(ScopeContext* parent_scope)
    : parent_scope(parent_scope),
      symbols() {
}

} // namespace fire::sema