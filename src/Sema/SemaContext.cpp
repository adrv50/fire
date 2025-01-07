#include <cassert>
#include "alert.h"
#include "Node.h"
#include "Sema.h"

namespace sema {

static Scope* _cur_func_keep = nullptr;

// ---------------------------------
//  SemaContext::SemaContext
// ---------------------------------
Scope* SemaContext::enter(Node* node) {
  this->cur_scope = this->cur_scope->find(node);

  assert(this->cur_scope);

  // this->cur_scope->letvp = this->cur_scope->variables.begin().base();

  if (node->is(ND_Function)) {
    _cur_func_keep = this->cur_func;

    this->cur_func = this->cur_scope;
  }

  return this->cur_scope;
}

// ---------------------------------
//  SemaContext::leave
// ---------------------------------
void SemaContext::leave() {
  if (this->cur_scope->node->is(ND_Function)) {
    this->cur_func = _cur_func_keep;
  }

  this->cur_scope = this->cur_scope->parent;
}

} // namespace sema