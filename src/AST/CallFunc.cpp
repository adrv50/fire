#include "AST.h"
#include "alert.h"

namespace fire::AST {

ASTPtr<CallFunc> CallFunc::New(ASTPointer expr, ASTVector args) {
  return ASTNew<CallFunc>(expr, std::move(args));
}

ASTPointer CallFunc::Clone() const {
  auto x = New(this->callee);

  for (ASTPointer const& a : this->args)
    x->args.emplace_back(a->Clone());

  x->callee_ast = this->callee_ast;
  x->callee_builtin = this->callee_builtin;

  return x;
}

ASTPtr<Class> CallFunc::get_class_ptr() const {
  return this->callee->GetID()->ast_class;
}

CallFunc::CallFunc(ASTPointer callee, ASTVector args)
    : Base(ASTKind::CallFunc, callee->token, callee->token),
      callee(callee),
      args(std::move(args)) {
}

} // namespace fire::AST