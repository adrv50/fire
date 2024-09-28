#include "alert.h"
#include "Sema/Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

TypeInfo Sema::EvalType(ASTPointer ast) {
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
          throw Error(x->token, "type '" + string(val) + "' cannot have parameters");
        }
        else if (c >= 1) {
          if ((int)x->type_params.size() < c)
            throw Error(x->token, "too few parameters");
          else if ((int)x->type_params.size() > c)
            throw Error(x->token, "too many parameters");
        }

        for (auto&& param : x->type_params) {
          type.params.emplace_back(this->EvalType(param));
        }

        return type;
      }
    }

    // type.name = x->GetName();
    // type.kind = TypeKind::Unknown;
    // return type;

    throw Error(x->token, "unknown type name");
  }

  case Kind::Value:
    return ast->as_value()->value->type;

  case Kind::Variable: {
    auto pvar = this->_find_variable(ast->GetID()->GetName());

    assert(pvar->is_type_deducted);

    return pvar->deducted_type;
  }

  case Kind::Array: {
    auto x = ast->As<AST::Array>();
    TypeInfo type = TypeKind::Vector;

    if (x->elements.empty()) {
      if (this->IsExpected(TypeKind::Vector)) {
        return x->elem_type = *this->GetExpectedType();
      }

      throw Error(x->token, "cannot deduction element type")
          .AddNote("you can use 'array' keyword for specify type. (like "
                   "\"array<T>([]...])\")");
    }

    auto const& elemType = type.params.emplace_back(this->EvalType(x->elements[0]));

    x->elem_type = elemType;

    for (auto it = x->elements.begin() + 1; it != x->elements.end(); it++) {
      if (!elemType.equals(this->EvalType(*it))) {
        throw Error(*it, "expected '" + elemType.to_string() +
                             "' type expression as element in array");
      }
    }

    return type;
  }

  case Kind::Identifier: {

    auto id = ASTCast<AST::Identifier>(ast);
    auto idinfo = this->get_identifier_info(id);

    auto& res = idinfo.result;

    switch (res.type) {
    case NameType::Var:
      id->kind = ASTKind::Variable;

      if (id->id_params.size() >= 1)
        throw Error(id->paramtok, "invalid use type argument for variable");

      if (!res.lvar->is_type_deducted)
        throw Error(ast->token, "cannot use variable before assignment");

      alertexpr(this->GetCurScope()->depth - res.lvar->depth);

      // distance
      id->depth = this->GetCurScope()->depth - res.lvar->depth;

      id->index = res.lvar->index;

      return res.lvar->deducted_type;

    case NameType::Func: {
      id->kind = ASTKind::FuncName;

      if (res.functions.size() >= 2) {
        throw Error(id->token, "function name '" + id->GetName() + "' is ambigous.");
      }

      auto func = res.functions[0];

      TypeInfo type = TypeKind::Function;

      type.params.emplace_back(this->EvalType(func->return_type));

      for (auto&& arg : func->arguments) {
        type.params.emplace_back(this->EvalType(arg->type));
      }

      return type;
    }

    case NameType::Unknown:
      throw Error(ast->token, "cannot find name '" + id->GetName() + "'");

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
      ast->kind = ASTKind::Variable;
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
      arg_types.emplace_back(this->EvalType(arg));
    }

    if (callee->kind == Kind::Identifier || callee->kind == Kind::ScopeResol) {
      IdentifierInfo idinfo =
          callee->kind == Kind::Identifier
              ? this->get_identifier_info(ASTCast<AST::Identifier>(callee))
              : this->get_identifier_info(ASTCast<AST::ScopeResol>(callee));

      if (idinfo.result.type != NameType::Func &&
          idinfo.result.type != NameType::BuiltinFunc) {
        throw Error(callee, "'" + idinfo.to_string() + "' is not a function");
      }

      ASTPtr<AST::Identifier> callee_as_id;

      if (callee->kind == Kind::Identifier)
        callee_as_id = ASTCast<AST::Identifier>(callee);
      else
        callee_as_id = ASTCast<AST::ScopeResol>(callee)->GetLastID();

      if (idinfo.result.type == NameType::BuiltinFunc) {
        alert;

        //
        // --- find built-in func
        //
        for (builtins::Function const* fn : idinfo.result.builtin_funcs) {
          auto res = this->check_function_call_parameters(
              call, fn->is_variable_args, fn->arg_types, arg_types, false);

          if (res.result == ArgumentCheckResult::Ok) {
            alertexpr(fn->name);

            call->callee_builtin = fn;

            return fn->result_type;
          }
        }
      }

      // 同じ名前の関数リスト
      ASTVec<AST::Function> const& hits = idinfo.result.functions;

      ASTVec<AST::Function> candidates; // 一致する関数を追加する

      // ヒットした関数の中から candidates に追加
      for (ASTPtr<AST::Function> func : hits) {
        // テンプレートの場合
        if (func->is_templated) {
          // パラメータの数が多すぎる場合はスキップ
          if (idinfo.id_params.size() > func->template_param_names.size())
            continue;

          // パラメータが少ない場合は引数からの推論を試みる

          // 引数の数が一致、かつインスタンス化に成功
          //  => 候補に追加
          if ((func->is_var_arg && call->args.size() + 1 >= func->arguments.size()) ||
              (call->args.size() == func->arguments.size())) {

            auto inst = this->Instantiate(func, call, idinfo, callee_as_id, arg_types);

            if (inst) {
              candidates.emplace_back(inst);
            }
          }

          continue;
        }

        TypeVec formal_arg_types;

        for (auto&& arg : func->arguments) {
          formal_arg_types.emplace_back(this->EvalType(arg->type));
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

        throw Error(callee, "function '" + idinfo.to_string() + "(" + arg_types_str +
                                ")" + "' is not defined");

        // TODO:
        // もし (hits.size() >= 1)
        // である場合、同じ名前の関数があること、引数間違いなどをヒントとして表示する
      }

      if (candidates.size() >= 2) {
        throw Error(callee, "call function '" + idinfo.to_string() + "' is ambigious");
      }

      call->callee_ast = candidates[0];

      return this->EvalType(candidates[0]->return_type);

    } // if Identifier or ScopeResol

    todo_impl;

    break;
  }

  case Kind::SpecifyArgumentName:
    return this->EvalType(ast->as_expr()->rhs);
  }

  if (ast->is_expr) {
    auto x = ast->as_expr();
    auto lhs = this->EvalType(x->lhs);
    auto rhs = this->EvalType(x->rhs);

    bool is_same = lhs.equals(rhs);

    // 基本的な数値演算を除外
    switch (ast->kind) {
    case Kind::Add:
      if (is_same && lhs.equals(TypeKind::String))
        return lhs;

    case Kind::Sub:
    case Kind::Mul:
    case Kind::Div:
      if (is_same && lhs.is_numeric()) {
        return lhs;
      }

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

    throw Error(x->op, "invalid operator for '" + lhs.to_string() + "' and '" +
                           rhs.to_string() + "'");
  }

  alertmsg(static_cast<int>(ast->kind));
  todo_impl;
}

TypeInfo Sema::ExpectType(TypeInfo const& type, ASTPointer ast) {
  this->_expected.emplace_back(type);

  if (auto t = this->EvalType(ast); !t.equals(type)) {
    throw Error(ast, "expected '" + type.to_string() + "' type expression, but found '" +
                         t.to_string() + "'");
  }

  return type;
}

TypeInfo* Sema::GetExpectedType() {
  if (this->_expected.empty())
    return nullptr;

  return &*this->_expected.rbegin();
}

bool Sema::IsExpected(TypeKind kind) {
  return !this->_expected.empty() && this->GetExpectedType()->kind == kind;
}

} // namespace fire::semantics_checker