#include "Sema/Sema.h"

namespace fire::semantics_checker {

IdentifierInfo Sema::GetIdentifierInfo(ASTPtr<AST::Identifier> Id, SemaContext& Ctx) {

  if (Ctx.MemberRefCtx.IsValid && Ctx.MemberRefCtx.Right == Id) {

    auto const& LeftII = Ctx.MemberRefCtx.LeftNameII;

    auto const& LeftType = Ctx.MemberRefCtx.LeftType;

    string name = Id->GetName();

    IdentifierInfo II = {.ast = Id};

    auto E = Ctx.MemberRefCtx.RefExpr;

    switch (LeftType.kind) {

    case TypeKind::Instance: {

      auto ClassPtr = ASTCast<AST::Class>(LeftType.type_ast);

      for (int i = 0; auto&& mv : ClassPtr->member_variables) {
        if (mv->GetName() == name) {

          II.result.type = NameType::MemberVar;

          Id->ast_class = ClassPtr;
          Id->index = i;

          E->kind = ASTKind::RefMemberVar;

          goto _found_member_var;
        }

        i++;
      }

      for (auto&& mf : ClassPtr->member_functions) {
        if (mf->GetName() == name) {
          Id->candidates.emplace_back(mf);
        }
      }

      if (Id->candidates.size() >= 1) {
        II.result.type = NameType::Func;
        E->kind = ASTKind::MemberFunction;
      }

    _found_member_var:

      break;
    }

    default:
      todo_impl;
    }

    return II;
  }

  return this->get_identifier_info(Id);
}

SemaIdentifierEvalResult Sema::EvalID(ASTPtr<AST::Identifier> id, SemaContext& Ctx) {

  SemaIdentifierEvalResult& SR =
      *EvaluatedIDResultRecord.emplace_back(std::make_shared<SemaIdentifierEvalResult>());

  TypeInfo& ST = SR.Type;

  SR.ast = id;

  switch (id->kind) {

  case ASTKind::Variable: {

    ST = id->lvar_ptr->deducted_type;

    break;
  }

  case ASTKind::FuncName: {

    ST = TypeKind::Function;

    ST.params = id->ft_args;
    ST.params.insert(ST.params.begin(), id->ft_ret);

    break;
  }

  case ASTKind::BuiltinFuncName: {

    ST = TypeKind::Function;

    auto bf = id->candidates_builtin[0];

    ST.is_free_args = bf->is_variable_args;

    ST.params = id->ft_args = bf->arg_types;

    ST.params.insert(ST.params.begin(), id->ft_ret = bf->result_type);

    break;
  }

  case ASTKind::Identifier: {

    auto& II = SR.II;

    II = this->GetIdentifierInfo(id, Ctx);

    auto& res = II.result;

    switch (res.type) {

    case NameType::Var: {

      id->kind = ASTKind::Variable;

      if (id->id_params.size() >= 1)
        throw Error(id->paramtok,
                    "cannot use template argument for '" + id->GetName() + "'");

      if (!res.lvar->is_type_deducted)
        throw Error(id, "cannot use variable before assignment");

      id->distance = this->GetCurScope()->depth - res.lvar->depth;

      id->index = res.lvar->index;
      id->index_add = res.lvar->index_add;

      id->lvar_ptr = res.lvar;

      if (res.lvar->decl)
        res.lvar->decl->index_add = id->index_add;

      ST = res.lvar->deducted_type;

      break;
    }

    case NameType::MemberVar: {

      id->kind = ASTKind::MemberVariable;

      ST = this->eval_type(id->ast_class->member_variables[id->index]->type);

      break;
    }

    case NameType::Func: {
      id->kind = ASTKind::FuncName;

      auto count =
          this->GetMatchedFunctions(id->candidates, res.functions, &II, &Ctx, true);

      if (count >= 2) {
        if (Ctx.FuncName.IsValid()) {
          todo_impl;
        }
      }

      else if (count == 0 && Ctx.FuncName.MustDecideOneCandidate) {
        throw Error(id->token, "no found function name '" + id->GetName() +
                                   "' matched template parameter <" +
                                   utils::join(", ", id->template_args,
                                               [](TypeInfo const& t) -> string {
                                                 return t.to_string();
                                               }) +
                                   ">");
      }

      TypeInfo type = TypeKind::Function;

      auto func = id->candidates[0];

      type.params.emplace_back(this->eval_type(func->return_type));

      for (auto&& arg : func->arguments)
        type.params.emplace_back(this->eval_type(arg->type));

      auto& _Record = this->InstantiatedRecords.emplace_back();

      _Record.Instantiated = func;

      auto& _Scope = _Record.Scope.emplace_back(func->GetScope());

      while (_Scope->_owner) {
        _Record.Scope.emplace_back(_Scope->_owner);

        _Scope = _Scope->_owner;
      }

      alert;

      ST = type;
      break;
    }

    case NameType::BuiltinFunc: {
      id->kind = ASTKind::BuiltinFuncName;

      id->candidates_builtin = std::move(res.builtin_funcs);

      if (id->candidates_builtin.size() >= 2) {
        if (Ctx.FuncName.MustDecideOneCandidate)
          throw Error(id->token, "function name '" + id->GetName() + "' is ambigous.");

        todo_impl; // may be overloaded.
      }

      assert(id->candidates_builtin.size() == 1);

      auto func = id->candidates_builtin[0];

      ST.is_free_args = func->is_variable_args;

      ST.params = id->ft_args = func->arg_types;
      ST.params.insert(ST.params.begin(), id->ft_ret = func->result_type);

      break;
    }

    case NameType::Class: {
      id->kind = ASTKind::ClassName;
      id->ast_class = res.ast_class;

      ST = TypeInfo::from_class(id->ast_class);

      break;
    }

    case NameType::Enum: {
      id->kind = ASTKind::EnumName;
      id->ast_enum = res.ast_enum;

      ST = TypeInfo::from_enum(id->ast_enum);

      break;
    }

    case NameType::TypeName: {
      ST = TypeInfo(res.kind, II.id_params);

      if (size_t C = ST.needed_param_count(); C == 0 && 1 <= ST.params.size()) {
        throw Error(id, "'" + id->GetName() + "' is not template");
      }
      else if (1 <= C && C != ST.params.size()) {
        throw Error(id, "no match template argument count");
      }

      break;
    }

    case NameType::Unknown:
      throw Error(id->token, "cannot find name '" + id->GetName() + "'");

    case NameType::Namespace:
      throw Error(id, "expected identifier-expression after this token ");

    default:
      alertexpr(static_cast<int>(res.type));
      todo_impl;
    }

    break;
  }

  default:
    todo_impl; // ?
  }

  if (Ctx.MemberRefCtx.IsValid && Ctx.MemberRefCtx.Left == id) {
    Ctx.MemberRefCtx.LeftNameII = SR.II;
    Ctx.MemberRefCtx.LeftType = ST;
  }

  return SR;
}

} // namespace fire::semantics_checker