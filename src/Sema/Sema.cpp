#include <iostream>
#include <sstream>
#include <list>

#include "alert.h"
#include "AST.h"

#include "Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

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

    _construct_scope_context(*S.child_scopes.emplace_back(c),
                             astfunc->block);

    this->add_func(astfunc);

    break;
  }

  case ASTKind::Enum:
    alert;
    this->add_enum(ASTCast<AST::Enum>(ast));
    break;

  case ASTKind::Class:
    this->add_class(ASTCast<AST::Class>(ast));
    break;
  }

  return 0;
}

ScopeContext::LocalVar* Sema::_find_variable(std::string const& name) {
  auto scope = this->scope_ptr.get();

  for (; scope->parent; scope = scope->parent) {
    for (auto&& lvar : scope->variables) {
      if (lvar.name == name)
        return &lvar;
    }
  }

  return nullptr;
}

ASTVec<Function> Sema::_find_func(std::string const& name) {
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

Sema::NameFindResult Sema::find_name(std::string const& name) {

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

void Sema::check_full() {

  this->_construct_scope_context(this->_scope_context, this->root);

  this->check(this->root);
}

void Sema::check(ASTPointer ast) {

  if (!ast) {
    return;
  }

  switch (ast->kind) {

  case ASTKind::Function: {
    auto x = ASTCast<AST::Function>(ast);

    auto func = this->get_func(x);

    assert(func);

    func->result_type = this->evaltype(x->return_type);

    for (auto&& arg : x->arguments)
      func->arg_types.emplace_back(this->evaltype(arg.type));

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

        if (auto c = type.needed_param_count();
            c == 0 && x->type_params.size() >= 1) {
          Error(x->token, "type '" + std::string(val) +
                              "' cannot have parameters")();
        }
        else if (c >= 1) {
          if (x->type_params.size() < c)
            Error(x->token, "too few parameters")();
          else if (x->type_params.size() > c)
            Error(x->token, "too many parameters")();
        }
      }
    }

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
        Error(id->token,
              "function name '" + id->GetName() + "' is ambigous.")();
      }

      auto func = res.functions[0];

      TypeInfo type = TypeKind::Function;

      type.params.emplace_back(this->evaltype(func->return_type));

      for (auto&& arg : func->arguments)
        type.params.emplace_back(this->evaltype(arg.type));

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
    case NameType::Var:
      todo_impl;
      break;

    case NameType::Enumerator: {
      TypeInfo type = TypeKind::Enumerator;
      type.name = scoperesol->first->GetName();
      alertmsg(type.name);
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
    auto x = ASTCast<AST::CallFunc>(ast);

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
        return (lhs.kind == TK::Float || rhs.kind == TK::Float) ? TK::Float
                                                                : TK::Int;

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
      if (lhs.is_hit({TK::Int, TK::Vector}) &&
          rhs.is_hit({TK::Int, TK::Vector}))
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