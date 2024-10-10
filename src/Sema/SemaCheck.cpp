#include <span>

#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

#include "ASTWalker.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

void Sema::check_full() {
  this->check(this->root);

  alertexpr(AST::ToString(this->root));

  // for (auto&& I : this->InstantiatedTemplates) {

  //   this->check(I);
  // }
}

void Sema::check(ASTPointer ast, Sema::SemaContext Ctx) {

  if (!ast)
    return;

  switch (ast->kind) {

  case ASTKind::Enum: {
    Vec<string_view> v;

    auto x = ASTCast<AST::Enum>(ast);

    for (auto&& e : x->enumerators) {
      if (utils::contains(v, e.name.str))
        throw Error(e.name, "duplicate enumerator name");
      else
        v.emplace_back(e.name.str);

      if (e.data_type == Enum::Enumerator::DataType::Value) {
        this->eval_type(e.types[0]);
      }
      else {
        Vec<string_view> v2;

        for (auto&& t : e.types) {
          auto arg = ASTCast<AST::Argument>(t);

          this->eval_type(arg->type);

          if (utils::contains(v2, arg->GetName()))
            throw Error(arg->name, "duplicate member variable name");
          else
            v2.emplace_back(arg->name.str);
        }
      }
    }

    break;
  }

  case ASTKind::Class: {

    Vec<string_view> v;

    auto x = ASTCast<AST::Class>(ast);

    for (auto&& mv : x->member_variables) {
      if (auto n = mv->GetName(); utils::contains(v, n))
        throw Error(mv->name, "duplicate member variable name");
      else
        v.emplace_back(n);

      this->check(mv->type);
      this->check(mv->init);
    }

    for (auto&& mf : x->member_functions) {
      this->check(mf);
    }

    break;
  }

  case ASTKind::Function: {
    auto x = ASTCast<AST::Function>(ast);

    // auto func = this->get_func(x);
    // SemaFunction* func = nullptr;

    if (x->is_templated) {
      break;
    }

    FunctionScope* func = nullptr;

    for (auto&& [_Key, _Val] : this->function_scope_map) {
      if (_Key == x) {
        func = _Val;
        break;
      }
    }

    auto& cur = this->GetCurScope();

    if (!func) {
      if (Ctx.original_template_func) {
        func = Ctx.original_template_func;
      }
    }

    // assert(func);

    auto pfunc = this->cur_function;
    this->cur_function = func;

    for (auto&& arg : func->arguments) {

      alertexpr(AST::ToString(arg.arg->type));

      arg.deducted_type = this->eval_type(arg.arg->type);
      arg.is_type_deducted = true;
    }

    auto RetType = this->eval_type(x->return_type);

    this->EnterScope(func);

    ASTVec<AST::Statement> return_stmt_list;

    AST::walk_ast(
        x->block,
        [&return_stmt_list](AST::ASTWalkerLocation loc, ASTPointer _ast) -> bool {
          if (loc == AST::AW_Begin && _ast->kind == ASTKind::Return) {
            return_stmt_list.emplace_back(ASTCast<AST::Statement>(_ast));
          }

          return true;
        });

    // if (!func->result_type.equals(TypeKind::None)) {
    //   if (func->return_stmt_list.empty()) {
    //     Error(func->ast->token, "function must return value of type '" +
    //                                 func->result_type.to_string() +
    //                                 "', but don't return "
    //                                 "anything.")
    //         .emit();

    //     throw Error(Error::ER_Note, func->ast->return_type, "specified here");
    //   }
    //   else if (auto block = func->block;
    //            (*block->ast->list.rbegin())->kind != ASTKind::Return) {
    //     throw Error(block->ast->endtok, "expected return-statement before this token");
    //   }
    // }

    this->check(x->block);

    for (auto&& rs : return_stmt_list) {
      if (rs->expr) {
        this->ExpectType(RetType, rs->expr);
      }
      else if (!RetType.equals(TypeKind::None)) {
        throw Error(rs->token, "expected '" + RetType.to_string() +
                                   "' type expression after this token");
      }
    }

    this->LeaveScope();

    this->cur_function = pfunc;

    break;
  }

  case ASTKind::Block:
  case ASTKind::Namespace: {
    auto x = ASTCast<AST::Block>(ast);

    this->EnterScope(x);

    for (size_t i = 0, end = x->list.size(); i < end; i++, (end = x->list.size())) {
      auto y = x->list[i];

      if (y->_s_pass_this)
        continue;

      alertexpr(static_cast<int>(y->kind));

      if (y->token.sourceloc.ref) {
        Error(y->token).format("element in block %p", x.get()).emit();
      }

      this->check(y);
    }

    this->LeaveScope();

    break;
  }

  case ASTKind::Vardef: {

    auto x = ASTCast<AST::VarDef>(ast);
    auto& curScope = this->GetCurScope();

    ScopeContext::LocalVar& var = ((BlockScope*)curScope)->variables[x->index];

    if (x->type) {
      var.deducted_type = this->eval_type(x->type);
      var.is_type_deducted = true;
    }

    if (x->init) {
      auto type = this->eval_type(x->init);

      if (x->type && !var.deducted_type.equals(type)) {
        throw Error(x->init, "expected '" + var.deducted_type.to_string() +
                                 "' type expression, but found '" + type.to_string() +
                                 "'");
      }

      var.deducted_type = type;
      var.is_type_deducted = true;
    }

    break;
  }

  case ASTKind::If: {
    auto d = ast->as_stmt()->data_if;

    if (!this->eval_type(d->cond).equals(TypeKind::Bool)) {
      throw Error(d->cond, "expected boolean expression");
    }

    this->check(d->if_true);
    this->check(d->if_false);

    break;
  }

  case ASTKind::Match: {
    auto x = ASTCast<AST::Match>(ast);

    auto cond = this->eval_type(x->cond);

    for (auto&& pattern : x->patterns) {
      auto px = pattern.expr;

      if (pattern.everything) {
        pattern.type = Match::Pattern::Type::AllCases;

        this->EnterScope(pattern.block);

        this->check(pattern.block);

        this->LeaveScope();
        continue;
      }

      auto var_scope = (BlockScope*)this->EnterScope(pattern.block);

      if (var_scope->variables.empty()) {
        alert;

        this->ExpectType(cond, px);
        this->check(pattern.block);

        pattern.type = Match::Pattern::Type::ExprEval;
        pattern.is_eval_expr = true;

        this->LeaveScope();
        continue;
      }

      alertexpr(var_scope);

      if (px->is_id_nonqual()) {
        auto& var = var_scope->variables[0];

        var.deducted_type = cond;
        var.is_type_deducted = true;

        this->eval_type(px);
        this->check(pattern.block);

        pattern.type = Match::Pattern::Type::Variable;
      }
      else {
        ASTPtr<AST::ScopeResol> enumerator = nullptr;
        std::span<ASTPointer> args;

        switch (px->kind) {
        case ASTKind::ScopeResol:
          enumerator = ASTCast<AST::ScopeResol>(px);
          pattern.type = Match::Pattern::Type::Enumerator;
          break;

        case ASTKind::CallFunc: {
          pattern.type = Match::Pattern::Type::EnumeratorWithArguments;

          auto cf = ASTCast<AST::CallFunc>(px);

          enumerator = ASTCast<AST::ScopeResol>(cf->callee);
          args = cf->args;

          if (enumerator->is(ASTKind::ScopeResol))
            enumerator->GetID()->sema_must_completed = false;

          auto e_type = this->eval_type(enumerator);

          alertexpr(static_cast<int>(e_type.kind));

          if (e_type.kind != TypeKind::Enumerator || e_type.type_ast != cond.type_ast) {
            todo_impl;
          }

          auto id = enumerator->GetID();
          auto& e = id->ast_enum->enumerators[id->index];

          switch (e.data_type) {
          case Enum::Enumerator::DataType::NoData:
            throw Error(id->token, "unexpected token '(' after this token");
            break;

          case Enum::Enumerator::DataType::Value: {
            if (args.empty()) {
              todo_impl;
            }
            else if (args.size() >= 2) {
              todo_impl;
            }

            if (var_scope->variables.size() >= 2) {
              throw Error(*id->token.get_next(),
                          "too many arguments for catch data of enumerator");
            }
            else if (var_scope->variables.size() == 1) {
              auto& var = var_scope->variables[0];

              var.deducted_type = this->eval_type(e.types[0]);
              var.is_type_deducted = true;
            }
            // else {
            //   this->ExpectType(this->eval_type(e.types[0]), args[0]);
            // }

            break;
          }

          case Enum::Enumerator::DataType::Structure: {
            if (args.size() != e.types.size()) {
              throw Error(enumerator,
                          "too " + string(args.size() < e.types.size() ? "few" : "many") +
                              " arguments to check matching of '" +
                              AST::ToString(enumerator) + "'");
            }

            for (size_t i = 0, j = 0; i < args.size(); i++) {
              auto et = this->eval_type(e.types[i]);

              if (!args[i]->is_id_nonqual()) {
                if (auto t_arg = this->eval_type(args[i]); !et.equals(t_arg)) {
                  throw Error(args[i], "expected '" + et.to_string() +
                                           "' type expression, but found '" +
                                           t_arg.to_string() + "'");
                }

                continue;
              }

              auto& v = var_scope->variables[j++];

              v.deducted_type = std::move(et);
              v.is_type_deducted = true;
            }

            break;
          }
          }

          this->check(pattern.block);

          break;
        }

        default:
          todo_impl; // invalid syntax
        }
      }

      this->LeaveScope();
    }

    break;
  }

  case ASTKind::Switch: {

    todo_impl;
  }

  case ASTKind::While: {
    auto d = ast->as_stmt()->data_while;

    this->check(d->cond);
    this->check(d->block);

    break;
  }

  case ASTKind::TryCatch: {
    auto d = ast->as_stmt()->data_try_catch;

    this->check(d->tryblock);

    vector<std::pair<ASTPointer, TypeInfo>> temp;

    for (auto&& c : d->catchers) {
      auto type = this->eval_type(c.type);

      for (auto&& c2 : d->catchers) {
        if (type.equals(c2._type)) {
          throw Error(c.type, "duplicated exception type")
              .AddChain(Error(Error::ER_Note, c2.type, "first defined here"));
        }
      }

      c._type = type;

      auto& e = ((BlockScope*)this->GetScopeOf(c.catched))->variables[0];

      e.name = c.varname.str;
      e.deducted_type = this->eval_type(c.type);
      e.is_type_deducted = true;

      this->check(c.catched);
    }

    break;
  }

  case ASTKind::Return: {
    // this->ExpectType(this->cur_function->result_type, ast->as_stmt()->expr);

    this->eval_type(ast->as_stmt()->expr);

    break;
  }

  case ASTKind::Throw: {
    this->check(ast->as_stmt()->expr);
    break;
  }

  case ASTKind::Break:
  case ASTKind::Continue:
    break;

  default:
    this->eval_type(ast);
  }
}
} // namespace fire::semantics_checker