#include "Object.h"
#include "Driver/Error.h"
#include "Builtins.h"
#include "Sema/Sema.h"
#include "Debug/Debug.h"

namespace fire::sema {

ScopeContext* Sema::find_scope(std::function<bool(ScopeContext*)> pred) {
  auto s = this->cur_scope;

  while (s && !pred(s))
    s = s->parent;

  return s;
}

ScopeContext* Sema::get_cur_func_scope() {
  return this->find_scope([](ScopeContext* s) {
    return s->kind == SC_Function;
  });
}

ScopeContext* Sema::get_cur_class_scope() {
  return this->find_scope([](ScopeContext* s) {
    return s->kind == SC_Class;
  });
}

ScopeContext* Sema::enter_scope(ScopeContext* scope) {
  assert(this->cur_scope->contains(scope));

  return this->cur_scope = scope;
}

void Sema::leave_scope() {
  this->cur_scope = this->cur_scope->parent;
}

size_t Sema::find_name(Vec<Symbol*>& out, string const& name, ScopeContext* start,
                       bool once) {
  if (!start)
    start = this->cur_scope;

  do {
    start->sym_table.find(out, name);

    if (once)
      goto __last;

    start = start->parent;
  } while (start && out.empty());

  if (out.empty()) {
    Builtins::Symbols::find(out, name);
  }

__last:;
  return out.size();
}

} // namespace fire::sema