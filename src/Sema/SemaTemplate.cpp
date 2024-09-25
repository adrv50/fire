#include "Sema.h"

namespace fire::semantics_checker {

void Sema::TemplateInstantiator::do_replace() {
}

Sema::TemplateInstantiator::TemplateInstantiator(ASTPtr<AST::Templatable> templatable) {
  this->ast_cloned = templatable->Clone();
}

} // namespace fire::semantics_checker