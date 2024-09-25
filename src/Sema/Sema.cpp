#include "Sema/Sema.h"

namespace fire::semantics_checker {

static vector<Sema*> _sema_instances;

Sema* Sema::GetInstance() {
  return *_sema_instances.rbegin();
}

Sema::Sema(ASTPtr<AST::Block> prg)
    : root(prg) {
  _sema_instances.emplace_back(this);

  this->_scope_context = new BlockScope(prg);
  this->_cur_scope = this->_scope_context;
}

Sema::~Sema() {
  _sema_instances.pop_back();
}

} // namespace fire::semantics_checker