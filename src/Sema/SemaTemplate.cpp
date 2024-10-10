#include "alert.h"
#include "Sema/Sema.h"

namespace fire::semantics_checker {

using TIContext = TemplateInstantiationContext;

bool TIContext::ParameterInfo::IsTypeDeductionCompleted() {
  if (!this->IsAssignmented)
    return false;

  for (auto&& p : this->Params)
    if (!p.IsTypeDeductionCompleted())
      return false;

  return true;
}

TIContext::ParameterInfo* TIContext::ParameterInfo::GetNotTypeDeducted() {

  if (!this->IsAssignmented)
    return this;

  for (auto&& p : this->Params) {
    if (auto x = p.GetNotTypeDeducted(); x)
      return x;
  }

  return nullptr;
}

TIContext::ParameterInfo::ParameterInfo(string const& Name, TypeInfo const& T)
    : Name(Name),
      Type(T) {
}

TIContext::ParameterInfo::ParameterInfo(Token const& token)
    : Name(string(token.str)),
      Tok(token) {
}

TIContext::ParameterInfo::ParameterInfo(AST::Templatable::ParameterName const& P)
    : ParameterInfo(P.token) {
  for (size_t i = 0; auto&& pp : P.params) {
    auto& e = this->Params.emplace_back(pp);

    if (this->TemplateArg) {
      e.TemplateArg = Sema::GetID(this->TemplateArg->id_params[i]);
    }

    i++;
  }
}

Error TIContext::CreateError() {
  switch (this->TryiedResult.Type) {
  case TI_NotTryied:
  case TI_Succeed:
    break;

  case TI_TooManyParameters:
    return Error(ParamsII->ast->token, "too many template arguments");

  case TI_CannotDeductType:
    return Error(TryiedResult.ErrParam->Tok, "cannot deduction the type of parameter '" +
                                                 TryiedResult.ErrParam->Name + "'");

  case TI_Arg_TypeMismatch:
    return Error(TryiedResult.ErrArgument,
                 "no match count of template-arguments to template parameter '" +
                     TryiedResult.ErrParam->Name + "'");

  case TI_Arg_TooFew:
    return Error(ParamsII->ast->token,
                 "too few arguments to call function '" + Item->GetName() + "'");

  case TI_Arg_TooMany:
    return Error(ParamsII->ast->token,
                 "too many arguments to call function '" + Item->GetName() + "'");
  }

  todo_impl;
}

TIContext::ParameterInfo* TIContext::_GetParam_in(Vec<ParameterInfo>& vp,
                                                  string const& name) {
  for (auto&& P : vp) {
    if (P.Name == name)
      return &P;

    if (auto p = _GetParam_in(P.Params, name); p)
      return p;
  }

  return nullptr;
}

TIContext::ParameterInfo* TIContext::GetParam(string const& name) {
  return _GetParam_in(this->Params, name);
}

size_t TIContext::GetAllParameters(Vec<ParameterInfo*>& Opv, ParameterInfo& Iv) {
  size_t count = 0;

  Opv.emplace_back(&Iv);

  count += 1;

  for (auto& _Input : Iv.Params) {
    count += GetAllParameters(Opv, _Input);
  }

  return count;
}

bool TIContext::CheckParameterMatchings(ParameterInfo& P) {
  if (P.Params.size() < P.Type.params.size()) {
    this->TryiedResult.Type = TI_TooManyParameters;
    this->TryiedResult.ErrParam = &P;
    return false;
  }
  else if (P.Params.size() > P.Type.params.size()) {
    this->TryiedResult.Type = TI_CannotDeductType;
    this->TryiedResult.ErrParam = &P.Params[0];
    return false;
  }

  if (P.TemplateArg) {
    for (size_t i = 0; i < P.Params.size(); i++) {
      P.Params[i].TemplateArg = Sema::GetID(P.TemplateArg->id_params[i]);
      P.Params[i].Type = P.Type.params[i];
    }
  }

  for (auto&& p : P.Params) {
    if (!CheckParameterMatchings(p))
      return false;
  }

  return true;
}

bool TIContext::CheckParameterMatchingToTypeDecl(ParameterInfo& P,
                                                 TypeInfo const& ArgType,
                                                 ASTPtr<AST::TypeName> T) {

  if (P.Name != T->GetName()) {
    return true;
  }

  if (P.Params.size() != T->type_params.size()) {
    this->TryiedResult.Type = TI_Arg_ParamError;
    this->TryiedResult.ErrArgument = T;
    this->TryiedResult.ErrParam = &P;
    return false;
  }

  for (size_t i = 0; i < P.Params.size(); i++) {
    if (!CheckParameterMatchingToTypeDecl(P.Params[i], ArgType.params[i],
                                          T->type_params[i])) {
      return false;
    }
  }

  return true;
}

bool TIContext::AssignmentTypeToParam(ParameterInfo* P, ASTPtr<AST::TypeName> TypeAST,
                                      TypeInfo const& type) {

  if (P->Name == TypeAST->GetName() && !P->IsAssignmented) {
    // P->Name = type.GetName();
    P->Type = type;
    P->IsAssignmented = true;
  }

  else if (!P->Type.equals(type)) {
    this->TryiedResult.Type = TI_Arg_TypeMismatch;
    this->TryiedResult.ErrParam = P;
    this->TryiedResult.Given = type;

    alertmsg("type mismatch");
    return false;
  }

  if (P->Params.size() != type.params.size()) {
    this->TryiedResult.Type = TI_Arg_ParamError;
    this->TryiedResult.ErrParam = P;
    this->TryiedResult.Given = type;
    alertmsg("no match param count of arg type");
    return false;
  }

  for (size_t i = 0; i < P->Params.size(); i++) {
    if (!AssignmentTypeToParam(&P->Params[i], TypeAST->type_params[i], type.params[i])) {
      alert;
      return false;
    }
  }

  return true;
}

ASTPtr<AST::Function>
TIContext::TryInstantiate_Of_Function(SemaFunctionNameContext* Ctx) {

  if (Item->kind != ASTKind::Function)
    return nullptr;

  auto Func = ASTCast<AST::Function>(Item);

  // 可変長じゃないのにテンプレート引数が多い
  if (!Func->IsVariableParameters &&
      Func->ParameterCount() < ParamsII->id_params.size()) {
    TryiedResult.Type = TI_TooManyParameters;
    return nullptr;
  }

  // 引数が与えられている文脈
  if (Ctx && Ctx->ArgTypes) {
    if (Ctx->GetArgumentsCount() < Func->arguments.size()) {
      this->TryiedResult.Type = TI_Arg_TooMany;
      this->Failed = true;
      return nullptr;
    }

    if (!Func->is_var_arg && Func->arguments.size() < Ctx->GetArgumentsCount()) {
      this->TryiedResult.Type = TI_Arg_TooFew;
      this->Failed = true;
      return nullptr;
    }

    for (size_t i = 0; auto&& ArgType : *Ctx->ArgTypes) {
      ASTPtr<AST::Argument>& Formal = Func->arguments[i];

      auto P = this->GetParam(string(Formal->type->GetName()));

      if (!P) {
        continue;
      }

      if (!CheckParameterMatchingToTypeDecl(*P, ArgType, Formal->type)) {
        this->Failed = true;
        return nullptr;
      }

      if (!AssignmentTypeToParam(P, Formal->type, ArgType)) {
        this->Failed = true;
        return nullptr;
      }

      i++;
    }
  }

  for (auto&& P : this->Params) {
    if (auto N = P.GetNotTypeDeducted(); N) {
      this->TryiedResult.ErrParam = N;
      this->TryiedResult.Type = TI_CannotDeductType;
      this->Failed = true;
      return nullptr;
    }
  }

  Func = ASTCast<AST::Function>(Func->Clone());

  AST::walk_ast(Func, [this](ASTWalkerLocation w, ASTPointer x) -> bool {
    if (w != AW_Begin)
      return false;

    if (auto t = x->As<AST::TypeName>(); t->kind == ASTKind::TypeName) {
      if (auto p = this->GetParam(t->GetName()); p) {
        t->name.str = p->Type.GetName();
      }
    }

    return true;
  });

  Func->is_templated = false;
  Func->is_instantiated = true;

  alertexpr("\n" << AST::ToString(Func));

  return Func;
}

ASTPtr<AST::Class> TIContext::TryInstantiate_Of_Class() {

  todo_impl;
}

TIContext::TemplateInstantiationContext(Sema& S, IdentifierInfo* ParamsII,
                                        ASTPtr<AST::Templatable> Item)
    : S(S),
      ParamsII(ParamsII),
      Item(Item) {
  for (auto&& P : Item->template_param_names) {
    this->Params.emplace_back(P);
  }

  for (size_t i = 0; TypeInfo const& ParamType : ParamsII->id_params) {
    auto& P = this->Params[i];

    P.Type = ParamType;
    P.TemplateArg = ParamsII->template_args[i].ast;

    if (!this->CheckParameterMatchings(P)) {
      this->Failed = true;
      break;
    }

    P.IsAssignmented = true;

    i++;
  }
}

size_t Sema::GetMatchedFunctions(ASTVec<AST::Function>& Matched,
                                 ASTVec<AST::Function> const& Candidates,
                                 IdentifierInfo* ParamsII, SemaContext* Ctx,
                                 bool ThrowError) {

  for (auto&& C : Candidates) {

    // テンプレート関数の場合、いったんインスタンス化してみる
    // 成功したら候補に追加する
    if (C->is_templated) {
      TemplateInstantiationContext TI{*this, ParamsII, C};

      ASTPtr<AST::Function> Instantiated = nullptr;

      if (!TI.Failed) {
        Instantiated = TI.TryInstantiate_Of_Function(Ctx ? &Ctx->FuncName : nullptr);
      }

      if (TI.Failed) {

        if (Candidates.size() == 1 && ThrowError) {

          Vec<TemplateInstantiationContext::ParameterInfo*> PP;

          for (auto&& P : TI.Params)
            TI.GetAllParameters(PP, P);

          throw TI.CreateError()
                .InLocation(
                    "in instantiation '" + C->GetName() + "<" +
                    utils::join(", ", C->template_param_names,
                                [](auto const& E) -> string {
                                  return E.to_string();
                                }) +
                    ">" + "(" +
                    utils::join(", ", C->arguments,
                                [](auto const& A) -> string {
                                  return AST::ToString(A);
                                }) +
                    ")" /*+ "' [" +
                    utils::join(  // === may be feature === //
                        ", ", PP,
                        [](TemplateInstantiationContext::ParameterInfo* PP) -> string {
                          return PP->Name + " = " +
                                 (PP->IsAssignmented ? PP->Type.to_string()
                                                     : "(unknown)");
                        }) +
                    "]"*/  )
                .AddChain(Error(Error::ER_Note, ParamsII->ast->token, "requested here"));
        }

        continue;
      }

      if (Instantiated) {

        Matched.emplace_back(Instantiated);

        continue;
      }
    }

    // 非テンプレート関数
    //  => 引数の型がわかる文脈であれば、引数との一致を見る
    else if (Ctx && Ctx->FuncName.IsValid()) {
      if (Ctx->FuncName.GetArgumentsCount() < C->arguments.size() ||
          (!C->is_var_arg && C->arguments.size() < Ctx->FuncName.GetArgumentsCount())) {
        continue;
      }

      for (size_t i = 0; i < C->arguments.size(); i++) {
        auto FormalType = this->eval_type(C->arguments[i]->type);
        auto const& ActualType = (*Ctx->FuncName.ArgTypes)[i];

        if (!FormalType.equals(ActualType)) {
          goto _pass_candidate;
        }
      }

      Matched.emplace_back(C);
    }

  _pass_candidate:;
  }

  return Matched.size();
}

} // namespace fire::semantics_checker

namespace fire::semantics_checker {

Sema::TemplateTypeApplier::Parameter&
Sema::TemplateTypeApplier::add_parameter(string const& name, TypeInfo type) {
  return this->parameter_list.emplace_back(name, std::move(type));
}

Sema::TemplateTypeApplier::Parameter*
Sema::TemplateTypeApplier::find_parameter(string const& name) {
  for (auto&& param : this->parameter_list)
    if (param.name == name)
      return &param;

  return nullptr;
}

Sema::TemplateTypeApplier::TemplateTypeApplier()
    : S(nullptr),
      ast(nullptr) {
}

Sema::TemplateTypeApplier::TemplateTypeApplier(ASTPtr<AST::Templatable> ast,
                                               vector<Parameter> parameter_list)
    : S(nullptr),
      ast(ast),
      parameter_list(std::move(parameter_list)) {
}

Sema::TemplateTypeApplier::~TemplateTypeApplier() {
  if (S) {
    alert;
    S->_applied_ptr_stack.pop_front();
  }
}

Error Sema::TemplateTypeApplier::make_error(ASTPointer) {

  panic;
}

TypeInfo* Sema::find_template_parameter_name(string const& name) {
  for (auto&& pt : this->_applied_ptr_stack) {
    if (auto p = pt->find_parameter(name); p)
      return &p->type;
  }

  return nullptr;
}

//
// args = template arguments (parameter)
//
Sema::TemplateTypeApplier Sema::apply_template_params(ASTPtr<AST::Templatable> ast,
                                                      TypeVec const& args) {
  TemplateTypeApplier apply{ast};

  for (size_t i = 0; i < args.size(); i++) {
    apply.add_parameter(ast->template_param_names[i].token.str, args[i]);
  }

  apply.result = TemplateTypeApplier::Result::NotChecked;

  return apply;
}

Sema::TemplateTypeApplier* Sema::_find_applied(TemplateTypeApplier const& t) {
  for (auto&& x : this->_applied_templates) {
    if (x.ast != t.ast)
      continue;

    if (x.parameter_list.size() != t.parameter_list.size())
      continue;

    for (size_t i = 0; i < t.parameter_list.size(); i++) {
      auto& A = x.parameter_list[i];
      auto& B = t.parameter_list[i];

      if (A.name != B.name || !A.type.equals(B.type)) {
        goto __skip;
      }
    }

    return &x;
  __skip:;
  }

  return nullptr;
}

} // namespace fire::semantics_checker