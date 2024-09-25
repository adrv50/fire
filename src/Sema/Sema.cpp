#include <iostream>
#include <sstream>
#include <list>

#include "alert.h"
#include "AST.h"

#include "Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))

#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

Sema::Sema(ASTPtr<AST::Block> prg)
    : root(prg),
      _scope_context(*this, nullptr, nullptr) {
}

ScopeContext::LocalVar* ScopeContext::get_var(string const& name) {
  for (auto&& var : this->variables)
    if (var.name == name)
      return &var;

  return nullptr;
}

std::tuple<bool, int, ScopeContext::LocalVar*>
ScopeContext::define_var(ASTPtr<VarDef> ast) {
  auto name = ast->GetName();

  if (auto pv = this->get_var(name); pv) {
    return std::make_tuple(true, pv->index, pv);
  }

  auto& var = this->variables.emplace_back(name);

  var.decl = ast;
  var.index = this->variables.size() - 1;

  if (ast->type) {
    var.is_type_deducted = true;
    var.type = this->S.evaltype(ast->type);
  }
  else if (ast->init) {
    var.is_type_deducted = true;
    var.type = this->S.evaltype(ast->init);
  }

  return std::make_tuple(false, var.index, &var);
}

/*
  -1  = ast is null
*/
int Sema::_construct_scope_context(ScopeContext& S, ASTPointer ast) {
  if (!ast)
    return -1;

  alert;
  switch (ast->kind) {
  case ASTKind::Vardef: {
    S.define_var(ASTCast<VarDef>(ast));
    break;
  }

  case ASTKind::Block: {
    auto x = ASTCast<Block>(ast);

    auto& c = S.child_scopes.emplace_back(new ScopeContext(*this, x, &S));

    for (auto&& y : x->list) {

      switch (y->kind) {
      case ASTKind::Vardef:
      case ASTKind::Function:
      case ASTKind::Enum:
      case ASTKind::Class:
        _construct_scope_context(*c, y);
        break;
      }
    }

    break;
  }

  case ASTKind::Function: {
    auto astfunc = ASTCast<Function>(ast);
    auto c = new ScopeContext(*this, astfunc->block, &S);

    alert;
    _construct_scope_context(*S.child_scopes.emplace_back(c), astfunc->block);

    alert;
    this->add_func(astfunc);

    break;
  }

  case ASTKind::Enum:
    this->add_enum(ASTCast<AST::Enum>(ast));
    break;

  case ASTKind::Class:
    this->add_class(ASTCast<AST::Class>(ast));
    break;
  }

  return 0;
}

ScopeContext::LocalVar* Sema::_find_variable(string const& name) {
  auto scope = this->scope_ptr.get();

  for (; scope->parent; scope = scope->parent) {
    for (auto&& lvar : scope->variables) {
      if (lvar.name == name)
        return &lvar;
    }
  }

  return nullptr;
}

ASTVec<Function> Sema::_find_func(string const& name) {
  ASTVec<Function> vec;

  for (auto&& f : this->functions)
    if (f.func->GetName() == name)
      vec.emplace_back(f.func);

  return vec;
}

ASTPtr<Enum> Sema::_find_enum(string const& name) {
  for (auto&& e : this->enums)
    if (e->GetName() == name)
      return e;

  return nullptr;
}

ASTPtr<Class> Sema::_find_class(string const& name) {
  for (auto&& c : this->classes)
    if (c->GetName() == name)
      return c;

  return nullptr;
}

Sema::NameFindResult Sema::find_name(string const& name) {

  NameFindResult result;

  result.lvar = this->_find_variable(name);

  if (result.lvar) {
    result.type = NameType::Var;
    return result;
  }

  result.functions = this->_find_func(name);

  if (result.functions.size() >= 1)
    result.type = NameType::Func;
  else if ((result.ast_enum = this->_find_enum(name)))
    result.type = NameType::Enum;
  else if ((result.ast_class = this->_find_class(name)))
    result.type = NameType::Class;

  return result;
}

string Sema::IdentifierInfo::to_string() const {
  string s = this->ast->GetName();

  if (!this->id_params.empty()) {
    s += "<";

    for (i64 i = 0; i < (i64)this->id_params.size(); i++) {
      s += this->id_params[i].to_string();

      if (i + 1 < (i64)this->id_params.size())
        s += ", ";
    }

    s += ">";
  }

  return s;
}

Sema::IdentifierInfo Sema::get_identifier_info(ASTPtr<AST::Identifier> ast) {
  IdentifierInfo id_info{};

  id_info.ast = ast;
  id_info.result = this->find_name(ast->GetName());

  for (auto&& x : ast->id_params)
    id_info.id_params.emplace_back(this->evaltype(x));

  return id_info;
}

// scope-resolution
Sema::IdentifierInfo Sema::get_identifier_info(ASTPtr<AST::ScopeResol> ast) {
  auto info = this->get_identifier_info(ast->first);

  string idname = ast->first->GetName();

  for (auto&& id : ast->idlist) {
    auto name = id->GetName();

    switch (info.result.type) {
    case NameType::Enum: {
      auto _enum = info.result.ast_enum;

      for (int _idx = 0; auto&& _e : _enum->enumerators->list) {
        if (_e->token.str == name) {
          info.ast = id;

          info.result.type = NameType::Enumerator;
          info.result.ast_enum = _enum;
          info.result.enumerator_index = _idx;
          info.result.name = name;

          goto _loop_continue;
        }

        _idx++;
      }

      Error(id->token, "enumerator '" + id->GetName() + "' is not found in enum '" +
                           _enum->GetName() + "'")();
    }

    case NameType::Class: {
      // auto _class = info.result.ast_class;

      todo_impl;
    }

    default:
      Error(id->token, "'" + idname + "' is not enum or class")();
    }

  _loop_continue:;
    idname += "::" + name;
  }

  return info;
}

void Sema::check_full() {
  alert;
  this->_construct_scope_context(this->_scope_context, this->root);

  alert;
  this->check(this->root);
}

void Sema::check(ASTPointer ast) {

  if (!ast) {
    return;
  }

  printkind;

  switch (ast->kind) {

  case ASTKind::Function: {
    alert;
    auto x = ASTCast<AST::Function>(ast);

    alert;
    auto func = this->get_func(x);

    alert;
    if (func->func->is_templated) {
      alert;
      break;
    }

    alert;
    assert(func != nullptr);

    func->result_type = this->evaltype(x->return_type);

    alert;
    AST::walk_ast(func->func->block,
                  [&func](AST::ASTWalkerLocation loc, ASTPointer _ast) {
                    if (loc == AST::AW_Begin && _ast->kind == ASTKind::Return) {
                      func->return_stmt_list.emplace_back(ASTCast<AST::Statement>(_ast));
                    }
                  });

    alert;
    for (auto&& arg : x->arguments) {
      alert;
      func->arg_types.emplace_back(this->evaltype(arg.type));
    }

    for (auto&& ret : func->return_stmt_list) {
      auto expr = ret->As<AST::Statement>()->get_expr();
      alert;

      if (auto type = this->evaltype(expr); !type.equals(func->result_type)) {
        alert;
        if (func->result_type.equals(TypeKind::None)) {
          Error(ret->token, "expected ';' after this token")();
        }
        else if (!expr) {
          Error(ret->token, "expected '" + func->result_type.to_string() +
                                "' type expression after this token")
              .emit();
        }
        else {
          Error(expr, "expected '" + func->result_type.to_string() +
                          "' type expression, but found '" + type.to_string() + "'")
              .emit();
        }

        goto _return_type_note;
      }
    }

    alert;
    if (!func->result_type.equals(TypeKind::None)) {
      if (func->return_stmt_list.empty()) {
        Error(func->func->token, "function must return value of type '" +
                                     func->result_type.to_string() +
                                     "', but don't return "
                                     "anything.")
            .emit();

      _return_type_note:
        Error(func->func->return_type, "specified here").emit(Error::ErrorLevel::Note);
      }
      else if (auto block = func->func->block;
               (*block->list.rbegin())->kind != ASTKind::Return) {
        Error(block->endtok, "expected return-statement before this token")();
      }
    }

    alert;
    this->check(x->block);

    break;
  }

  case ASTKind::Block: {
    auto x = ASTCast<AST::Block>(ast);

    this->scope_ptr.enter(x);

    for (auto&& e : x->list)
      this->check(e);

    this->scope_ptr.leave();

    break;
  }

  case ASTKind::Vardef: {

    auto x = ASTCast<AST::VarDef>(ast);

    this->check(x->type);
    this->check(x->init);

    break;
  }

  default:
    this->evaltype(ast);
  }
}

TypeInfo Sema::evaltype(ASTPointer ast) {
  using Kind = ASTKind;
  using TK = TypeKind;

  if (!ast)
    return {};

  printkind;

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

    type.name = x->GetName();
    type.kind = TypeKind::Unknown;

    return type;
  }

  case Kind::Value:
    return ast->as_value()->value->type;

  case Kind::Variable:
    todo_impl;
    break;

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

      return res.lvar->type;

    case NameType::Func: {
      id->kind = ASTKind::FuncName;

      if (res.functions.size() >= 2) {
        Error(id->token, "function name '" + id->GetName() + "' is ambigous.")();
      }

      auto func = res.functions[0];

      TypeInfo type = TypeKind::Function;

      type.params.emplace_back(this->evaltype(func->return_type));

      for (auto&& arg : func->arguments) {
        type.params.emplace_back(this->evaltype(arg.type));
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
      for (auto&& func : hits) {
        // テンプレートの場合
        if (func->is_templated) {
          // パラメータの数が多すぎる場合はスキップ
          if (idinfo.id_params.size() > func->template_param_names.size())
            continue;

          // 引数の数だけみて、もし一致すれば
          if ((func->is_var_arg && call->args.size() + 1 >= func->arguments.size()) ||
              (call->args.size() == func->arguments.size())) {
            alert;
            // candidates.emplace_back(func);

            std::map<string /* = name*/, std::pair<ASTPtr<AST::TypeName>, TypeInfo>>
                param_types;

            // _Replacer = 再帰関数;
            // 対象の TypeInfo::name が param_types の KEY として存在する場合、
            // その対象の TypeInfo の .kind を VALUE で置き換える。
            // そしてそれを TypeInfo::params に対しても、同様の処理を再帰的に行う関数。
            auto _Replacer = [this, &param_types](auto _Replacer_func_ptr,
                                                  TypeInfo _Target) -> TypeInfo {
              alert;
              for (auto&& [_name, _type_pair] : param_types) {
                auto&& [_type_ast, _typeinfo] = _type_pair;

                if (_typeinfo.kind == TypeKind::Unknown && _Target.name == _name) {
                  alert;
                  _Target.kind = _typeinfo.kind;
                }
              }

              for (TypeInfo& _Param_In_Target : _Target.params) {
                alert;
                _Param_In_Target = _Replacer_func_ptr(_Replacer_func_ptr, _Target);
              }

              return _Target;
            };

            for (i64 i = 0; i < (i64)callee_as_id->id_params.size(); i++) {
              // ASTPtr<AST::TypeName> actual_param = callee_as_id->id_params[i];

              // formal_param = パラメータ ("T" とか) のトークンへのポインタ
              string formal_param_name = func->template_param_names[i].str;

              alert;
              ASTPtr<AST::TypeName> actual_parameter_type = callee_as_id->id_params[i];

              alert;
              if (param_types.contains(formal_param_name)) {
                Error(actual_parameter_type->token, "redefined the parameter name '" +
                                                        actual_parameter_type->token.str +
                                                        "'")();
              }

              alert;
              param_types[formal_param_name] = std::make_pair(
                  actual_parameter_type, this->evaltype(actual_parameter_type));
            }

            alert;
            ASTPtr<AST::Function> cloned_func =
                ASTCast<AST::Function>(func->Clone()); // Instantiate.

            cloned_func->is_templated = false;

            alert;
            call->callee_ast = cloned_func;

            alert;
            AST::walk_ast(call->callee_ast, [&param_types](AST::ASTWalkerLocation _loc,
                                                           ASTPointer _ast) {
              alert;
              if (_loc != AST::AW_Begin)
                return;

              alert;
              if (_ast->kind == ASTKind::TypeName) {
                alert;
                ASTPtr<AST::TypeName> _ast_type = ASTCast<AST::TypeName>(_ast);
                string name = _ast_type->GetName();

                alert;
                alertmsg(name);

                if (param_types.contains(name)) {
                  _ast_type->name.str = param_types[name].second.to_string();

                  alertmsg(_ast_type->name.str);
                  // todo_impl;
                }
              }
            });

            alert;
            this->add_func(cloned_func);

            alert;
            this->check(cloned_func);

            return this->evaltype(cloned_func->return_type);
          }

          continue;
        }

        TypeVec formal_arg_types;

        for (auto&& arg : func->arguments) {
          formal_arg_types.emplace_back(this->evaltype(arg.type));
        }

        auto res = this->check_function_call_parameters(
            call, func->is_var_arg, formal_arg_types, arg_types, !func->is_templated);

        if (res.result == ArgumentCheckResult::Ok)
          candidates.emplace_back(func);
      }

      // 一致する候補がない
      alert;
      if (candidates.empty()) {
        alert;
        string arg_types_str;

        for (i64 i = 0; i < (i64)arg_types.size(); i++) {
          arg_types_str += arg_types[i].to_string();
          if (i + 1 < (i64)arg_types.size())
            arg_types_str += ", ";
        }

        Error(callee, "function '" + idinfo.to_string() + "(" + arg_types_str + ")" +
                          "' is not defined")();
      }

      alert;
      if (candidates.size() >= 2) {
        Error(callee, "call function '" + idinfo.to_string() + "' is ambigious")();
      }

      alert;
      call->callee_ast = candidates[0];

      alert;
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