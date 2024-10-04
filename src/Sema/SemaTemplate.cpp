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

ASTPtr<AST::Function>
Sema::instantiate_template_func(ASTPtr<AST::Function> func, ASTPointer reqeusted,
                                ASTPtr<AST::Identifier> id, ASTVector args,
                                TypeVec const& arg_types, bool ignore_args) {

  if (func->is_var_arg ? args.size() < func->arguments.size()
                       : func->arguments.size() != args.size())
    return nullptr;

  InstantiateRequest req{
      .requested = reqeusted,
      .original = func,
      .arg_types = arg_types,
  };

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

    ASTPointer actual_parameter_type = id->id_params[i];

    param_types[formal_param_name] =
        InstantiateRequest::Argument{.type = this->eval_type(actual_parameter_type),
                                     .given = actual_parameter_type,
                                     .is_deducted = true};
  }

  //
  // 引数の型から推論する
  for (size_t i = 0; i < func->arguments.size(); i++) {

    ASTPtr<AST::Argument> formal_arg = func->arguments[i];
    TypeInfo const& actual_type = arg_types[i];

    // formal_arg_begin   = 引数を定義してる構文へのポインタ
    // arg_type           = 渡された引数の型

    string param_name = formal_arg->type->GetName();

    if (!param_types[param_name].is_deducted) {

      auto& _Param = param_types[param_name];

      _Param.guess = args[i];
      _Param.type = actual_type;

      _Param.is_deducted = true;
    }
  }

  //
  // 推論できていないパラメータがある場合、エラー
  for (auto&& [_name, _data] : param_types) {
    if (!_data.is_deducted) {
      throw Error(reqeusted,
                  "the type of template parameter '" + _name + "' is not deducted yet");
    }
  }

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
        _ast_type->name.str = param_types[name].type.without_params().to_string();
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

  if (ignore_args) {
    todo_impl;
  }

  TypeVec formal_arg_types;

  for (auto&& arg : req.cloned->arguments) {
    formal_arg_types.emplace_back(this->eval_type(arg->type));
  }

  auto res = this->check_function_call_parameters(args, req.cloned->is_var_arg,
                                                  formal_arg_types, arg_types, false);

  switch (res.result) {
  case ArgumentCheckResult::TypeMismatch:
    throw Error(args[res.index], "expected '" + formal_arg_types[res.index].to_string() +
                                     "' type expression, but found '" +
                                     arg_types[res.index].to_string() + "'");

  case ArgumentCheckResult::Ok: {
    req.result_type = this->eval_type(req.cloned->return_type);

    if (auto found = this->find_request_of_func(func, req.result_type, arg_types);
        !found) {
      alert;
      this->ins_requests.emplace_back(req);
    }
    else {
      return found->cloned;
    }

    return req.cloned;
  }
  }

  return nullptr;
}

} // namespace fire::semantics_checker