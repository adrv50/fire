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

  for (auto&& TI_Record : this->InstantiatedRecords) {
    this->GetHistory() = TI_Record.Scope;

    this->check(TI_Record.Instantiated);
  }

  // alertexpr(AST::ToString(this->root));

  // for (auto&& I : this->InstantiatedTemplates) {

  //   this->check(I);
  // }
}

void Sema::check(ASTPointer ast, SemaContext& Ctx) {

  if (!ast)
    return;

  switch (ast->kind) {

  case ASTKind::Enum: {
    Vec<string> v;

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
        Vec<string> v2;

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

    Vec<string> v;

    auto x = ASTCast<AST::Class>(ast);

    auto context = Ctx;

    context.ClassCtx.Now_Analysing = x;

    if (auto IHBaseName = x->InheritBaseClassName; IHBaseName) {

      if (!IHBaseName->is_ident_or_scoperesol()) {
        throw Error(IHBaseName, "invalid syntax");
      }

      this->eval_type(IHBaseName, context);

      if (IHBaseName->kind != ASTKind::ClassName) {
        throw Error(IHBaseName,
                    "'" + AST::ToString(IHBaseName) + "' is not a class name");
      }

      auto BaseClass = IHBaseName->GetID()->ast_class;

      if (BaseClass == x) {
        throw Error(x->InheritBaseClassName, "cannot inherit self class");
      }

      if (BaseClass->IsFinal) {
        throw Error(IHBaseName,
                    "cannot inheritance the final class '" + BaseClass->GetName() + "'");
      }

      BaseClass->InheritedBy.emplace_back(x);

      x->InheritBaseClassPtr = BaseClass;

      context.ClassCtx.InheritBaseClass = BaseClass;

      this->check(BaseClass);
    }

    for (auto&& mv : x->member_variables) {
      if (auto n = mv->GetName(); utils::contains(v, n))
        throw Error(mv->name, "duplicate member variable name");
      else
        v.emplace_back(n);

      this->check(mv->type);
      this->check(mv->init);
    }

    for (auto&& mf : x->member_functions) {
      this->check(mf, context);

      if (mf->is_virtualized) {
        x->VirtualFunctions.emplace_back(mf);
      }

      else if (mf->is_override) {

        string name = mf->GetName();

        auto Base = context.ClassCtx.InheritBaseClass;

        ASTPtr<Function> ov_base = nullptr;

        do {

          Vec<ASTPtr<Function>> ov;

          for (auto&& base_f : Base->member_functions) {
            if (base_f->is_virtualized && base_f->GetName() == name) {
              ov.emplace_back(base_f);
            }
          }

          for (auto it = ov.begin(); it != ov.end();) {
            auto& ov_f = *it;

            if (ov_f->is_var_arg != mf->is_var_arg ||
                ov_f->arguments.size() != mf->arguments.size() ||
                !this->eval_type(ov_f->return_type, context)
                     .equals(this->eval_type(mf->return_type, context))) {
              ov.erase(it);
              continue;
            }

            it++;
          }

          if (ov.size() == 1) {
            ov_base = ov[0];
            break;
          }

          if (ov.size() >= 2) {
            // error: ambiguity
            todo_impl;
          }

          Base = Base->InheritBaseClassPtr;
        } while (Base);

        if (!ov_base) {
          throw Error(mf->override_specify_tok,
                      "function " + name +
                          " must be overridden from a base class, but the same "
                          "name cannot be found in the base class");
        }

        mf->OverridingFunc = ov_base;
      }
    }

    break;
  }

  case ASTKind::Function: {
    auto x = ASTCast<AST::Function>(ast);

    // auto func = this->get_func(x);
    // SemaFunction* func = nullptr;

    if (x->IsTemplated) {

      break;
    }

    // FunctionScope* func = nullptr;

    FunctionScope* func = x->GetScope();

    // for (auto&& [_Key, _Val] : this->function_scope_map) {
    //   if (_Key == x) {
    //     func = _Val;
    //     break;
    //   }
    // }

    // if (!func) {
    //   if (Ctx.original_template_func) {
    //     func = Ctx.original_template_func;
    //   }
    // }

    assert(func);

    if (x->is_virtualized) {
    }

    auto pfunc = this->cur_function;
    this->cur_function = func;

    for (auto&& arg : func->arguments) {

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

    this->check(x->block);

    this->LeaveScope();

    for (auto&& rs : return_stmt_list) {
      if (rs->expr) {
        this->ExpectType(RetType, rs->expr);
      }
      else if (!RetType.equals(TypeKind::None)) {
        throw Error(rs->token, "expected '" + RetType.to_string() +
                                   "' type expression after this token");
      }
    }

    this->cur_function = pfunc;

    break;
  }

  case ASTKind::Block:
  case ASTKind::Namespace: {
    auto x = ASTCast<AST::Block>(ast);

    this->EnterScope(x);

    for (auto&& e : x->list)
      this->check(e, Ctx);

    this->LeaveScope();

    break;
  }

  case ASTKind::Vardef: {

    auto x = ASTCast<AST::VarDef>(ast);
    auto& curScope = this->GetCurScope();

    LocalVar& var = ((BlockScope*)curScope)->variables[x->index];

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

      if (px->IsUnqualifiedIdentifier()) {
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

          // if (enumerator->Is(ASTKind::ScopeResol))
          //   enumerator->GetID()->sema_must_completed = false;

          auto e_type = this->eval_type(enumerator);

          // alertexpr(static_cast<int>(e_type.kind));

          // if (e_type.kind != TypeKind::Enumerator || e_type.type_ast != cond.type_ast)
          // {
          //   todo_impl;
          // }

          if (enumerator->kind != ASTKind::Enumerator) {
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

              if (!args[i]->IsUnqualifiedIdentifier()) {
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