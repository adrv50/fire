#include "AST.h"

namespace fire::AST {

sema::FunctionScope* Function::GetScope() {
  return (sema::FunctionScope*)(this->ScopeCtxPtr);
}

ASTPtr<Function> Function::New(Token tok, Token name) {
  return ASTNew<Function>(tok, name);
}

ASTPtr<Function> Function::New(Token tok, Token name, ASTVec<Argument> args,
                               bool is_var_arg, ASTPtr<TypeName> rettype,
                               ASTPtr<Block> block) {
  return ASTNew<Function>(tok, name, std::move(args), is_var_arg, rettype, block);
}

ASTPtr<Argument>& Function::add_arg(Token const& tok, ASTPtr<TypeName> type) {
  return this->arguments.emplace_back(Argument::New(tok, type));
}

ASTPtr<Argument> Function::find_arg(std::string const& name) {
  for (auto&& arg : this->arguments)
    if (arg->GetName() == name)
      return arg;

  return nullptr;
}

ASTPointer Function::Clone() const {
  auto x = New(this->token, this->name);

  x->_Copy(*this);

  for (auto&& arg : this->arguments) {
    x->arguments.emplace_back(ASTCast<Argument>(arg->Clone()));
  }

  if (this->return_type)
    x->return_type = ASTCast<TypeName>(this->return_type->Clone());

  x->block = ASTCast<Block>(this->block->Clone());
  x->is_var_arg = this->is_var_arg;

  return x;
}

Function::Function(Token tok, Token name)
    : Function(tok, name, {}, false, nullptr, nullptr) {
}

Function::Function(Token tok, Token name, ASTVec<Argument> args, bool is_var_arg,
                   ASTPtr<TypeName> rettype, ASTPtr<Block> block)
    : Templatable(ASTKind::Function, tok, name),
      arguments(args),
      return_type(rettype),
      block(block),
      is_var_arg(is_var_arg) {
}

} // namespace fire::AST