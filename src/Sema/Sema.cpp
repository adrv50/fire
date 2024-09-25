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

static vector<Sema*> _sema_instances;

Sema* Sema::GetInstance() {
  return *_sema_instances.rbegin();
}

Sema::Sema(ASTPtr<AST::Block> prg)
    : root(prg) {
  _sema_instances.emplace_back(this);

  this->_scope_context = new BlockScope(prg);
}

Sema::~Sema() {
  _sema_instances.pop_back();
}

ScopeContext::LocalVar* Sema::_find_variable(string const& name) {

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

} // namespace fire::semantics_checker