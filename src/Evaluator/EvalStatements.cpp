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

    stack->func_result = this->evaluate(stmt->expr);

    for (auto&& s : this->var_stack) {
      s->returned = true;

      if (s == stack) {
        break;
      }
    }

    break;
  }

  case Kind::Throw:
    throw this->evaluate(ast->as_stmt()->expr);

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
    auto d = ast->as_stmt()->data_if;

    auto cond = this->evaluate(d->cond);

    if (cond->get_vb())
      this->evaluate(d->if_true);
    else
      this->evaluate(d->if_false);

    break;
  }

  case Kind::Match: {
    auto x = ASTCast<AST::Match>(ast);

    auto cond = this->evaluate(x->cond);

    for (auto&& P : x->patterns) {
      switch (P.type) {
      case AST::Match::Pattern::Type::ExprEval: {
        if (cond->Equals(this->evaluate(P.expr))) {
          this->push_stack(0);
          break;
        }

        continue;
      }

      case AST::Match::Pattern::Type::Variable: {
        auto S = this->push_stack(1);

        S->var_list.emplace_back(cond);

        // this->eval_stmt(P.block);

        // this->pop_stack();
        break;
      }

      case AST::Match::Pattern::Type::Enumerator: {

        // this is Type::ExprEval ... ?

        todo_impl;

        break;
      }

      case AST::Match::Pattern::Type::EnumeratorWithArguments: {

        auto iter = P.vardef_list.begin();

        auto cf = P.expr->As<AST::CallFunc>();

        auto eor_id = cf->callee->GetID();

        auto ep = eor_id->ast_enum;
        auto ei = eor_id->index;

        auto e_ref = ep->enumerators[ei];

        auto stack = this->push_stack(P.vardef_list.size());

        auto obj_to_cmp = PtrCast<ObjEnumerator>(cond->Clone());

        if (e_ref.data_type == AST::Enum::Enumerator::DataType::Value) {
          stack->var_list[0] = obj_to_cmp->data;
        }
        else {
          auto& list = obj_to_cmp->data->As<ObjIterable>()->list;

          for (size_t i = 0, j = 0; i < cf->args.size(); i++) {
            if (iter != P.vardef_list.end() && iter->first == i) {
              stack->var_list[j++] = list[i];
              iter++;
            }
            else {
              if (!this->evaluate(cf->args[i])->Equals(list[i])) {
                goto _match_failure;
              }
            }
          }
        }

        break;
      }

      case AST::Match::Pattern::Type::AllCases: {
        this->push_stack(0);
        break;
      }
      }

      this->eval_stmt(P.block);

      this->pop_stack();
      break;

    _match_failure:;
      this->pop_stack();
    }

    break;
  }

  case Kind::While: {
    auto d = ast->as_stmt()->data_while;

    while (this->evaluate(d->cond)->get_vb()) {
      this->evaluate(d->block);
    }

    break;
  }

  case Kind::TryCatch: {
    auto d = ast->as_stmt()->data_try_catch;

    auto s1 = this->var_stack;
    auto s2 = this->call_stack;
    auto s3 = this->loops;

    try {
      this->evaluate(d->tryblock);
    }
    catch (ObjPointer obj) {
      this->var_stack = s1;
      this->call_stack = s2;
      this->loops = s3;

      for (auto&& c : d->catchers) {
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

    if (x->init) {
      this->get_cur_stack().var_list[x->index + x->index_add] = this->evaluate(x->init);
    }

    break;
  }
  }
}

} // namespace fire::eval