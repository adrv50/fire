#include "alert.h"
#include "Sema/Sema.h"
#include "Error.h"
#include "Builtin.h"

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
          throw Error(x->token,
                      "type '" + string(val) + "' cannot have parameters");
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

  case Kind::Value: {
    return ast->as_value()->value->type;
  }

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

    auto const& elemType =
        type.params.emplace_back(this->EvalType(x->elements[0]));

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

    //
    // TypeKind::Function:
    //
    // type.params =
    //   { return-type, arg1, arg2, ...(arg) }
    //

    switch (res.type) {
    case NameType::Var: {
      id->kind = ASTKind::Variable;

      if (id->id_params.size() >= 1)
        throw Error(id->paramtok, "invalid use type argument for variable");

      if (!res.lvar->is_type_deducted)
        throw Error(ast->token, "cannot use variable before assignment");

      // distance
      id->depth = this->GetCurScope()->depth - res.lvar->depth;

      id->index = res.lvar->index;

      return res.lvar->deducted_type;
    }

    case NameType::Func: {
      id->kind = ASTKind::FuncName;

      id->candidates = std::move(idinfo.result.functions);

      if (id->candidates.size() >= 2) {
        if (!id->sema_allow_ambigious)
          throw Error(id->token,
                      "function name '" + id->GetName() + "' is ambigous.");

        return TypeKind::Function; // ambigious -> called by case CallFunc
      }

      assert(id->candidates.size() == 1);

      TypeInfo type = TypeKind::Function;

      ASTPtr<AST::Function> func = id->candidates[0];

      if (!func->is_templated) {
        type.params.emplace_back(this->EvalType(func->return_type));

        for (auto&& arg : func->arguments) {
          type.params.emplace_back(this->EvalType(arg->type));
        }
      }

      return type;
    }

    case NameType::BuiltinFunc: {
      id->kind = ASTKind::BuiltinFuncName;

      id->candidates_builtin = std::move(idinfo.result.builtin_funcs);

      if (id->candidates_builtin.size() >= 2) {
        if (!id->sema_allow_ambigious)
          throw Error(id->token,
                      "function name '" + id->GetName() + "' is ambigous.");

        return TypeKind::Function; // ambigious -> called by case CallFunc
      }

      assert(id->candidates_builtin.size() == 1);

      TypeInfo type = TypeKind::Function;

      builtins::Function const* func = id->candidates_builtin[0];

      type.params = func->arg_types;
      type.params.insert(type.params.begin(), func->result_type);

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

    ASTPointer functor = call->callee;

    ASTPtr<AST::Identifier> id = nullptr; // -> functor (if id or scoperesol)

    if (functor->is_ident_or_scoperesol()) {
      id = Sema::GetID(functor);

      id->sema_allow_ambigious = true;
    }

    TypeInfo functor_type = this->EvalType(functor);

    TypeVec arg_types;

    for (ASTPointer arg : call->args) {
      arg_types.emplace_back(this->EvalType(arg));
    }

    switch (functor->kind) {
      //
      // ユーザー定義の関数名:
      //
    case ASTKind::FuncName: {

      IdentifierInfo info = this->get_identifier_info(id);

      ASTVec<AST::Function> final_candidates;

      for (ASTPtr<AST::Function> candidate : id->candidates) {
        if (candidate->is_templated) {
          if (info.id_params.size() > candidate->template_param_names.size())
            continue;

          if (candidate->arguments.size() == call->args.size() ||
              (candidate->is_var_arg &&
               candidate->arguments.size() <= call->args.size() + 1)) {

            auto instantiated =
                this->Instantiate(candidate, call, info, id, arg_types);

            if (instantiated != nullptr)
              final_candidates.emplace_back(instantiated);
          }
        }
        else {
          TypeVec formal_arg_types;

          for (ASTPointer arg : candidate->arguments)
            formal_arg_types.emplace_back(this->EvalType(arg));

          auto res = this->check_function_call_parameters(
              call, candidate->is_var_arg, formal_arg_types, arg_types, false);

          if (res.result == ArgumentCheckResult::Ok) {
            final_candidates.emplace_back(candidate);
          }
        }
      }

      if (final_candidates.empty()) {
        throw Error(functor, "a function '" + info.to_string() + "(" +
                                 utils::join<TypeInfo>(", ", arg_types,
                                                       [](TypeInfo t) {
                                                         return t.to_string();
                                                       }) +
                                 ")" + "' is not defined");
      }

      else if (final_candidates.size() >= 2) {
        throw Error(functor,
                    "function name '" + info.to_string() + "' is ambigious");
      }

      call->callee_ast = final_candidates[0];

      return this->EvalType(final_candidates[0]->return_type);
    }

    case ASTKind::BuiltinFuncName: {

      for (builtins::Function const* fn : id->candidates_builtin) {
        auto res = this->check_function_call_parameters(
            call, fn->is_variable_args, fn->arg_types, arg_types, false);

        if (res.result == ArgumentCheckResult::Ok) {
          call->callee_builtin = fn;

          return fn->result_type;
        }
      }

      break;
    }
    }

    throw Error(functor, "expected function name or callable expression");
  }

  case Kind::SpecifyArgumentName:
    return this->EvalType(ast->as_expr()->rhs);

  case Kind::IndexRef: {
    auto x = ASTCast<AST::Expr>(ast);

    auto arr = this->EvalType(x->lhs);

    switch (arr.kind) {
    case TypeKind::Vector:
      return arr.params[0];
    }

    throw Error(x->op, "'" + arr.to_string() + "' type is not subscriptable");
  }

  case Kind::MemberAccess: {
    todo_impl;
  }

  case Kind::Not: {
    auto x = ASTCast<AST::Expr>(ast);

    auto type = this->EvalType(x->lhs);

    if (type.kind != TypeKind::Bool)
      throw Error(x->lhs, "expected boolean expression");

    return type;
  }

  case Kind::Assign: {
    auto x = ASTCast<AST::Expr>(ast);

    auto src = this->EvalType(x->rhs);

    if (x->lhs->kind == ASTKind::Identifier ||
        x->lhs->kind == ASTKind::ScopeResol) {
      auto idinfo = this->get_identifier_info(x->lhs);

      if (auto lvar = idinfo.result.lvar; idinfo.result.type == NameType::Var) {

        lvar->deducted_type = src;
        lvar->is_type_deducted = true;
      }
    }

    auto dest = this->EvalType(x->lhs);

    if (!dest.equals(src)) {
      throw Error(x->rhs,
                  "expected '" + dest.to_string() + "' type expression");
    }

    if (!this->IsWritable(x->lhs)) {
      throw Error(x->lhs, "expected writable expression");
    }

    return dest;
  }
  }

  if (ast->is_expr) {
    auto x = ast->as_expr();
    auto lhs = this->EvalType(x->lhs);
    auto rhs = this->EvalType(x->rhs);

    bool is_same = lhs.equals(rhs);

    // 基本的な数値演算を除外
    switch (ast->kind) {
    case Kind::Add:
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
      if (is_same && lhs.is_numeric_or_char() && rhs.is_numeric_or_char())
        return TK::Bool;

      break;

    case Kind::LogAND:
    case Kind::LogOR:
      if (lhs.kind != TypeKind::Bool || rhs.kind != TypeKind::Bool)
        break;

    case Kind::Equal:
      return TK::Bool;

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
      //  --> append element to vector
      if (lhs.kind == TK::Vector || rhs.kind == TK::Vector)
        return TK::Vector;

      //
      // char + char  <--  Invalid
      // char + str
      // str  + char
      // str  + str
      if (lhs.is_char_or_str() && rhs.is_char_or_str())
        return TK::String;

      break;

    case Kind::Mul:
      //
      // int * str
      // str * int
      //  => str
      if (!is_same && lhs.is_hit_kind({TK::Int, TK::String}) &&
          rhs.is_hit_kind({TK::Int, TK::String}))
        return TK::String;

      // vector * int
      // int * vector
      //  => vector
      if (!is_same && lhs.is_hit_kind({TK::Int, TK::Vector}) &&
          rhs.is_hit_kind({TK::Int, TK::Vector}))
        return TK::Vector;

      break;
    }

    throw Error(x->op, "invalid operator '" + x->op.str + "' for '" +
                           lhs.to_string() + "' and '" + rhs.to_string() + "'");
  }

  alertmsg(static_cast<int>(ast->kind));
  todo_impl;
}

} // namespace fire::semantics_checker