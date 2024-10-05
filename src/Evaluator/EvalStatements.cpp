#include "Builtin.h"
#include "Evaluator.h"
#include "Error.h"

#define CAST(T) auto x = ASTCast<AST::T>(ast)

namespace fire::eval {

void Evaluator::eval_stmt(ASTPointer ast) {
  using Kind = ASTKind;

  if (!ast) {
    return;
  }

  switch (ast->kind) {

  case Kind::Return: {
    auto stmt = ast->as_stmt();

    auto& stack = *this->call_stack.begin();

    stack->func_result = this->evaluate(stmt->get_expr());

    for (auto&& s : this->var_stack) {
      s->returned = true;

      if (s == stack) {
        break;
      }
    }

    break;
  }

  case Kind::Throw:
    throw this->evaluate(ast->as_stmt()->get_expr());

  case Kind::Break:
    (*this->loops.begin())->breaked = true;
    break;

  case Kind::Continue:
    (*this->loops.begin())->continued = true;
    break;

  case Kind::Block: {
    CAST(Block);

    auto stack = this->push_stack(x->stack_size);

    for (auto&& y : x->list) {
      this->evaluate(y);

      if (stack->returned)
        break;
    }

    this->pop_stack();

    break;
  }

  case Kind::Namespace: {
    CAST(Block);

    for (auto&& y : x->list) {
      this->evaluate(y);
    }

    break;
  }

  case Kind::If: {
    auto d = ast->as_stmt()->get_data<AST::Statement::If>();

    auto cond = this->evaluate(d.cond);

    if (cond->get_vb())
      this->evaluate(d.if_true);
    else
      this->evaluate(d.if_false);

    break;
  }

  case Kind::While: {
    auto d = ast->as_stmt()->get_data<AST::Statement::While>();

    while (this->evaluate(d.cond)->get_vb()) {
      this->evaluate(d.block);
    }

    break;
  }

  case Kind::TryCatch: {
    auto d = ast->as_stmt()->get_data<AST::Statement::TryCatch>();

    auto s1 = this->var_stack;
    auto s2 = this->call_stack;
    auto s3 = this->loops;

    try {
      this->evaluate(d.tryblock);
    }
    catch (ObjPointer obj) {
      this->var_stack = s1;
      this->call_stack = s2;
      this->loops = s3;

      for (auto&& c : d.catchers) {
        if (c._type.equals(obj->type)) {
          auto s = this->push_stack(1);

          s->var_list = {obj};

          for (auto&& x : c.catched->list) {
            this->evaluate(x);

            if (s->returned || s->breaked || s->continued)
              break;
          }

          this->pop_stack();

          return;
        }
      }

      throw obj;
    }

    break;
  }

  case Kind::Vardef: {
    CAST(VarDef);

    if (x->init)
      this->get_cur_stack().var_list[x->index] = this->evaluate(x->init);

    break;
  }
  }
}

} // namespace fire::eval