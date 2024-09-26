#include "alert.h"
#include "Sema/Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

TypeInfo Sema::evaltype(ASTPointer ast) {
  using Kind = ASTKind;
  using TK = TypeKind;

  if (!ast)
    return {};

  switch (ast->kind) {
  case Kind::Enum:
  case Kind::Function:
    return {};

  case Kind::TypeName: {
    // clang-format off
    static std::map<TypeKind, char const*> kind_name_map {
      { TypeKind::None,       "none" },
      { TypeKind::Int,        "int" },
      { TypeKind::Float,      "float" },
      { TypeKind::Size,       "usize" },
      { TypeKind::Bool,       "bool" },
      { TypeKind::Char,       "char" },
      { TypeKind::String,     "string" },
      { TypeKind::Vector,     "vector" },
      { TypeKind::Tuple,      "tuple" },
      { TypeKind::Dict,       "dict" },
      { TypeKind::Instance,   "instance" },
      { TypeKind::Module,     "module" },
      { TypeKind::Function,   "function" },
      { TypeKind::Module,     "module" },
      { TypeKind::TypeName,   "type" },
    };
    // clang-format on

    auto x = ASTCast<AST::TypeName>(ast);

    TypeInfo type;

    for (auto&& [key, val] : kind_name_map) {
      if (val == x->GetName()) {
        type = key;

        if (int c = type.needed_param_count();
            c == 0 && (int)x->type_params.size() >= 1) {
          Error(x->token, "type '" + string(val) + "' cannot have parameters")();
        }
        else if (c >= 1) {
          if ((int)x->type_params.size() < c)
            Error(x->token, "too few parameters")();
          else if ((int)x->type_params.size() > c)
            Error(x->token, "too many parameters")();
        }

        return type;
      }
    }

    // type.name = x->GetName();
    // type.kind = TypeKind::Unknown;
    // return type;

    Error(x->token, "unknown type name")();
  }

  case Kind::Value:
    return ast->as_value()->value->type;

  case Kind::Variable: {
    auto pvar = this->_find_variable(ast->GetID()->GetName());

    assert(pvar->is_type_deducted);

    return pvar->deducted_type;
  }

  case Kind::Identifier: {

    auto id = ASTCast<AST::Identifier>(ast);
    auto idinfo = this->get_identifier_info(id);

    auto& res = idinfo.result;

    switch (res.type) {
    case NameType::Var:
      id->kind = ASTKind::Variable;

      if (id->id_params.size() >= 1)
        Error(id->paramtok, "invalid use type argument for variable")();

      if (!res.lvar->is_type_deducted)
        Error(ast->token, "cannot use variable before assignment")();

      return res.lvar->deducted_type;

    case NameType::Func: {
      id->kind = ASTKind::FuncName;

      if (res.functions.size() >= 2) {
        Error(id->token, "function name '" + id->GetName() + "' is ambigous.")();
      }

      auto func = res.functions[0];

      TypeInfo type = TypeKind::Function;

      type.params.emplace_back(this->evaltype(func->return_type));

      for (auto&& arg : func->arguments) {
        type.params.emplace_back(this->evaltype(arg->type));
      }

      return type;
    }

    case NameType::Unknown:
      Error(ast->token, "cannot find name '" + id->GetName() + "'")();

    default:
      todo_impl;
    }

    break;
  }

  case Kind::ScopeResol: {
    auto scoperesol = ASTCast<AST::ScopeResol>(ast);
    auto idinfo = this->get_identifier_info(scoperesol);

    switch (idinfo.result.type) {
    case NameType::Var: {
      todo_impl;
      break;
    }

    case NameType::Enumerator: {
      ast->kind = Kind::Enumerator;

      TypeInfo type = TypeKind::Enumerator;
      type.name = scoperesol->first->GetName();
      return type;
    }

    case NameType::Unknown:
      todo_impl;
      break;
    }

    todo_impl;

    break;
  }

  case Kind::CallFunc: {
    auto call = ASTCast<AST::CallFunc>(ast);

    auto callee = call->callee;

    TypeInfo type = TypeKind::Function;

    TypeVec arg_types;

    for (ASTPointer arg : call->args) {
      arg_types.emplace_back(this->evaltype(arg));
    }

    if (callee->kind == Kind::Identifier || callee->kind == Kind::ScopeResol) {
      IdentifierInfo idinfo =
          callee->kind == Kind::Identifier
              ? this->get_identifier_info(ASTCast<AST::Identifier>(callee))
              : this->get_identifier_info(ASTCast<AST::ScopeResol>(callee));

      if (idinfo.result.type != NameType::Func) {
        Error(callee, "'" + idinfo.to_string() + "' is not a function")();
      }

      ASTPtr<AST::Identifier> callee_as_id;

      if (callee->kind == Kind::Identifier)
        callee_as_id = ASTCast<AST::Identifier>(callee);
      else
        callee_as_id = *ASTCast<AST::ScopeResol>(callee)->idlist.rbegin();

      ASTVec<AST::Function> const& hits = idinfo.result.functions;

      // テンプレート以外で一致する場合追加する
      ASTVec<AST::Function> candidates;

      // ヒットした関数の中から candidates に追加
      for (ASTPtr<AST::Function> func : hits) {
        // テンプレートの場合
        if (func->is_templated) {
          // パラメータの数が多すぎる場合はスキップ
          if (idinfo.id_params.size() > func->template_param_names.size())
            continue;

          // パラメータが少ない場合は引数からの推論を試みる

          // 引数の数だけみて、もし一致すれば
          if ((func->is_var_arg && call->args.size() + 1 >= func->arguments.size()) ||
              (call->args.size() == func->arguments.size())) {
            // candidates.emplace_back(func);

            struct _Data {
              TypeInfo type;

              ASTPtr<AST::TypeName> ast = nullptr; // @<...> で渡される型名
              // ScopeContext::LocalVar* lvar=nullptr;

              bool is_deducted = false;
            };

            //
            // < string = name , >
            std::map<string, _Data> param_types;

            for (auto&& param : func->template_param_names) {
              param_types[param.str] = {};
            }

            //
            // 呼び出し式の、関数名のあとにある "@<...>" の構文から、
            // 明示的に指定されている型をパラメータに当てはめる。
            //
            for (i64 i = 0; i < (i64)callee_as_id->id_params.size(); i++) {
              string formal_param_name = func->template_param_names[i].str;

              ASTPtr<AST::TypeName> actual_parameter_type = callee_as_id->id_params[i];

              param_types[formal_param_name] =
                  _Data{.type = this->evaltype(actual_parameter_type),
                        .ast = actual_parameter_type,
                        .is_deducted = true};
            }

            //
            // 引数の型から推論する
            for (auto formal_arg_begin = func->arguments.begin();
                 auto&& arg_type : arg_types) {

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
                Error(call, "the type of template parameter '" + _name +
                                "' is not deducted yet")();
              }
            }

            ASTPtr<AST::Function> cloned_func =
                ASTCast<AST::Function>(func->Clone()); // Instantiate.

            // original.
            FunctionScope* template_func_scope = (FunctionScope*)this->GetScopeOf(func);

            assert(template_func_scope);

            // instantiated.
            FunctionScope* instantiated_func_scope =
                template_func_scope->AppendInstantiated(cloned_func);

            cloned_func->is_templated = false;

            AST::walk_ast(cloned_func, [&param_types, instantiated_func_scope](
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

            call->callee_ast = cloned_func;

            this->SaveScopeInfo();

            this->BackToDepth(template_func_scope->depth - 1);

            this->_cur_scope = this->_scope_history.emplace_back(instantiated_func_scope);

            this->check(cloned_func);

            this->RestoreScopeInfo();

            TypeVec formal_arg_types;

            for (auto&& arg : cloned_func->arguments) {
              formal_arg_types.emplace_back(this->evaltype(arg->type));
            }

            auto res = this->check_function_call_parameters(
                call, cloned_func->is_var_arg, formal_arg_types, arg_types, false);

            if (res.result == ArgumentCheckResult::Ok) {
              candidates.emplace_back(cloned_func);
            }

            else {
              switch (res.result) {
              case ArgumentCheckResult::TypeMismatch:
                Error(call->args[res.index],
                      "expected '" + formal_arg_types[res.index].to_string() +
                          "' type expression, but found '" +
                          arg_types[res.index].to_string() + "'")();
              }
            }
          }

          continue;
        }

        TypeVec formal_arg_types;

        for (auto&& arg : func->arguments) {
          formal_arg_types.emplace_back(this->evaltype(arg->type));
        }

        auto res = this->check_function_call_parameters(
            call, func->is_var_arg, formal_arg_types, arg_types, false);

        if (res.result == ArgumentCheckResult::Ok)
          candidates.emplace_back(func);
      }

      // 一致する候補がない
      if (candidates.empty()) {
        string arg_types_str;

        for (i64 i = 0; i < (i64)arg_types.size(); i++) {
          arg_types_str += arg_types[i].to_string();
          if (i + 1 < (i64)arg_types.size())
            arg_types_str += ", ";
        }

        Error(callee, "function '" + idinfo.to_string() + "(" + arg_types_str + ")" +
                          "' is not defined")();

        // TODO:
        // もし (hits.size() >= 1)
        // である場合、同じ名前の関数があること、引数間違いなどをヒントとして表示する
      }

      if (candidates.size() >= 2) {
        Error(callee, "call function '" + idinfo.to_string() + "' is ambigious").emit();

        for (auto&& C : hits) {
          Error(C->name, "uoo").emit();
        }

        todo_impl;
      }

      call->callee_ast = candidates[0];

      return this->evaltype(candidates[0]->return_type);

    } // if Identifier or ScopeResol

    todo_impl;

    break;
  }

  case Kind::SpecifyArgumentName:
    return this->evaltype(ast->as_expr()->rhs);
  }

  if (ast->is_expr) {
    auto x = ast->as_expr();
    auto lhs = this->evaltype(x->lhs);
    auto rhs = this->evaltype(x->rhs);

    bool is_same = lhs.equals(rhs);

    // 基本的な数値演算を除外
    switch (ast->kind) {
    case Kind::Add:
    case Kind::Sub:
    case Kind::Mul:
    case Kind::Div:
      if (lhs.is_numeric() && rhs.is_numeric())
        return (lhs.kind == TK::Float || rhs.kind == TK::Float) ? TK::Float : TK::Int;

      break;

    case Kind::Mod:
    case Kind::LShift:
    case Kind::RShift:
      if (lhs.kind == TK::Int && rhs.kind == TK::Int)
        return TK::Int;

      break;

    case Kind::Bigger:
    case Kind::BiggerOrEqual:
      if (lhs.is_numeric_or_char() && rhs.is_numeric_or_char()) {
      case Kind::Equal:
      case Kind::LogAND:
      case Kind::LogOR:
        return TK::Bool;
      }

      break;

    case Kind::BitAND:
    case Kind::BitXOR:
    case Kind::BitOR:
      if (lhs.kind == TK::Int && rhs.kind == TK::Int)
        return TK::Int;

      break;
    }

    switch (ast->kind) {
    case Kind::Add:
      //
      // vector + T
      // T + vector
      if (lhs.kind == TK::Vector || rhs.kind == TK::Vector)
        return TK::Vector;

      //
      // char + char  <--  Invalid
      // char + str
      // str  + char
      // str  + str
      if (!is_same && lhs.is_char_or_str() && rhs.is_char_or_str())
        return TK::String;

      break;

    case Kind::Mul:
      //
      // int * str
      // str * int
      //  => str
      if (!is_same && lhs.is_hit({TK::Int, TK::String}) &&
          rhs.is_hit({TK::Int, TK::String}))
        return TK::String;

      // vector * int
      // int * vector
      //  => vector
      if (lhs.is_hit({TK::Int, TK::Vector}) && rhs.is_hit({TK::Int, TK::Vector}))
        return TK::Vector;

      break;
    }

    Error(x->op, "invalid operator for '" + lhs.to_string() + "' and '" +
                     rhs.to_string() + "'")();
  }

  alertmsg(static_cast<int>(ast->kind));
  todo_impl;
}

} // namespace fire::semantics_checker