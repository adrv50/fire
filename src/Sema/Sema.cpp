#include "Sema/Sema.h"

namespace fire::semantics_checker {

static vector<Sema*> _sema_instances;

Sema* Sema::GetInstance() {
  return *_sema_instances.rbegin();
}

Sema::Sema(ASTPtr<AST::Block> prg)
    : root(prg) {
  _sema_instances.emplace_back(this);

  this->_scope_context = new BlockScope(0, prg);

  this->_location.Current = this->_scope_context;
}

Sema::~Sema() {
  _sema_instances.pop_back();

  delete this->_scope_context;
}

} // namespace fire::semantics_checker