#include "Builtin.h"
#include "Evaluator.h"
#include "Error.h"

#define CAST(T) auto x = ASTCast<AST::T>(ast)

namespace fire::eval {

ObjPtr<ObjNone> _None;

Evaluator::Evaluator() {
  _None = ObjNew<ObjNone>();
}

Evaluator::~Evaluator() {
}

Evaluator::VarStack& Evaluator::push_stack(int var_size) {

  auto stack = this->var_stack.emplace_back(std::make_shared<VarStack>());

  stack->var_list.reserve((size_t)var_size);

  return *stack;
}

void Evaluator::pop_stack() {
  this->var_stack.pop_back();
}

Evaluator::VarStack& Evaluator::get_cur_stack() {
  return **this->var_stack.rbegin();
}

Evaluator::VarStack& Evaluator::get_stack(int distance) {
  auto it = this->var_stack.rbegin();

  for (int i = 0; i < distance; i++)
    it++;

  return **it;
}

ObjPointer Evaluator::evaluate(ASTPointer ast) {
  using Kind = ASTKind;

  if (!ast) {
    return _None;
  }

  switch (ast->kind) {

  case Kind::Function:
  case Kind::Class:
  case Kind::Enum:
    break;

  case Kind::Value:
    return ast->as_value()->value;

  case Kind::Variable: {
    auto x = ast->GetID();

    return this->get_stack(x->depth /*distance*/).var_list[x->index];
  }

  case Kind::CallFunc: {
    CAST(CallFunc);

    ObjVector args;

    for (auto&& arg : x->args)
      args.emplace_back(this->evaluate(arg));

    if (x->callee_builtin) {
      return x->callee_builtin->Call(x, std::move(args));
    }

    auto& stack = this->push_stack(x->args.size());

    stack.var_list = std::move(args);

    alertexpr(this->var_stack.size());

    this->evaluate(x->callee_ast->block);

    auto result = stack.func_result;

    this->pop_stack();

    return result ? result : _None;
  }

  case Kind::Return: {
    auto stmt = ast->as_stmt();

    alertexpr(stmt->ret_func_scope_distance);

    this->get_stack(stmt->ret_func_scope_distance).func_result =
        this->evaluate(stmt->get_expr());

    break;
  }

  case Kind::Block: {
    CAST(Block);

    this->push_stack(x->stack_size);

    for (auto&& y : x->list)
      this->evaluate(y);

    this->pop_stack();

    break;
  }

  default:
    if (ast->is_expr)
      return this->eval_expr(ASTCast<AST::Expr>(ast));

    alertexpr(static_cast<int>(ast->kind));
    Error(ast->token, "@@@").emit();

    todo_impl;
  }

  return _None;
}

ObjPointer Evaluator::eval_expr(ASTPtr<AST::Expr> ast) {
  using Kind = ASTKind;

  CAST(Expr);

  auto lhs = this->evaluate(x->lhs)->Clone();
  auto rhs = this->evaluate(x->rhs);

  switch (x->kind) {

  case Kind::Add: {

    switch (lhs->type.kind) {
    case TypeKind::Int:
      lhs->As<ObjPrimitive>()->vi += rhs->As<ObjPrimitive>()->vi;
      break;

    case TypeKind::Float:
      lhs->As<ObjPrimitive>()->vf += rhs->As<ObjPrimitive>()->vf;
      break;

    case TypeKind::String:
      lhs->As<ObjString>()->AppendList(PtrCast<ObjIterable>(rhs));
      break;

    default:
      todo_impl;
    }

    break;
  }

  default:
    todo_impl;
  }

  return lhs;
}

} // namespace fire::eval