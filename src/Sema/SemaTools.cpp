#include <sstream>
#include <list>

#include "alert.h"
#include "AST.h"

#include "Sema.h"
#include "Error.h"

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

ScopeContext* Sema::GetCurScope() {
  return this->_cur_scope;
}

ScopeContext* Sema::EnterScope(ASTPointer ast) {
}

void Sema::LeaveScope(ASTPointer ast) {
}

} // namespace fire::semantics_checker