#include "Sema/Sema.h"

namespace fire::semantics_checker {

Sema::InstantiateRequest* Sema::find_request_of_func(ASTPtr<AST::Function> func,
                                                     TypeInfo ret_type, TypeVec args) {

  for (auto&& r : this->ins_requests) {
    if (r.original == func && r.result_type.equals(ret_type)) {
      if (r.arg_types.size() == args.size()) {
        for (i64 i = 0; i < (i64)args.size(); i++) {
          if (!r.arg_types[i].equals(args[i]))
            return nullptr;
        }

        return &r;
      }
    }
  }

  return nullptr;
}

ASTPtr<AST::Function> Sema::Instantiate(ASTPtr<AST::Function> func,
                                        ASTPtr<AST::CallFunc> call, IdentifierInfo idinfo,
                                        ASTPtr<AST::Identifier> id,
                                        TypeVec const& arg_types) {

  InstantiateRequest req{
      .requested = call,
      .idinfo = idinfo,
      .original = func,
      .arg_types = arg_types,
  };

  req.idinfo = idinfo;

  auto& param_types = req.param_types;

  for (auto&& param : func->template_param_names) {
    param_types[param.str] = {};
  }

  //
  // 呼び出し式の、関数名のあとにある "@<...>" の構文から、
  // 明示的に指定されている型をパラメータに当てはめる。
  //
  for (i64 i = 0; i < (i64)id->id_params.size(); i++) {
    string formal_param_name = func->template_param_names[i].str;

    ASTPtr<AST::TypeName> actual_parameter_type = id->id_params[i];

    param_types[formal_param_name] =
        InstantiateRequest::Argument{.type = this->evaltype(actual_parameter_type),
                                     .ast = actual_parameter_type,
                                     .is_deducted = true};
  }

  //
  // 引数の型から推論する
  for (auto formal_arg_begin = func->arguments.begin(); auto&& arg_type : arg_types) {

    // formal_arg_begin   = 引数を定義してる構文へのポインタ
    // arg_type           = 渡された引数の型

    string param_name = (*formal_arg_begin)->type->GetName();

    if (!param_types[param_name].is_deducted) {

      auto& _Param = param_types[param_name];

      _Param.ast = (*formal_arg_begin)->type;
      _Param.type = arg_type;
      _Param.is_deducted = true;
    }

    formal_arg_begin++;
  }

  //
  // 推論できていないパラメータがある場合、エラー
  for (auto&& [_name, _data] : param_types) {
    if (!_data.is_deducted) {
      throw Error(call,
                  "the type of template parameter '" + _name + "' is not deducted yet");
    }
  }

  req.requested = call;

  req.cloned = ASTCast<AST::Function>(func->Clone()); // Instantiate.

  // original.
  FunctionScope* template_func_scope = (FunctionScope*)this->GetScopeOf(func);

  assert(template_func_scope);

  // instantiated.
  FunctionScope* instantiated_func_scope =
      template_func_scope->AppendInstantiated(req.cloned);

  req.cloned->is_templated = false;

  AST::walk_ast(req.cloned, [&param_types, instantiated_func_scope](
                                AST::ASTWalkerLocation _loc, ASTPointer _ast) {
    if (_loc != AST::AW_Begin) {
      return;
    }

    if (_ast->kind == ASTKind::Argument) {
      auto _arg = ASTCast<AST::Argument>(_ast);

      auto pvar = instantiated_func_scope->find_var(_arg->GetName());

      pvar->is_type_deducted = true;
      pvar->deducted_type = param_types[_arg->type->GetName()].type;
    }

    if (_ast->kind == ASTKind::TypeName) {
      ASTPtr<AST::TypeName> _ast_type = ASTCast<AST::TypeName>(_ast);
      string name = _ast_type->GetName();

      if (param_types.contains(name)) {
        _ast_type->name.str = param_types[name].type.to_string();
      }
    }
  });

  // call->callee_ast = cloned_func;

  this->SaveScopeLocation();

  this->BackToDepth(template_func_scope->depth - 1);

  req.scope_loc = ScopeLocation{.Current = instantiated_func_scope,
                                .History = this->_location.History};

  req.scope_loc.History.emplace_back(req.scope_loc.Current);

  this->RestoreScopeLocation();

  TypeVec formal_arg_types;

  for (auto&& arg : req.cloned->arguments) {
    formal_arg_types.emplace_back(this->evaltype(arg->type));
  }

  auto res = this->check_function_call_parameters(call, req.cloned->is_var_arg,
                                                  formal_arg_types, arg_types, false);

  switch (res.result) {
  case ArgumentCheckResult::TypeMismatch:
    throw Error(call->args[res.index], "expected '" +
                                           formal_arg_types[res.index].to_string() +
                                           "' type expression, but found '" +
                                           arg_types[res.index].to_string() + "'");

  case ArgumentCheckResult::Ok: {
    req.result_type = this->evaltype(req.cloned->return_type);

    if (!this->find_request_of_func(func, req.result_type, arg_types)) {
      alert;

      this->ins_requests.emplace_back(req);
    }

    return req.cloned;
  }
  }

  return nullptr;
}

} // namespace fire::semantics_checker