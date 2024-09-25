#include <list>

#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

struct ScopeInfoSave {
  ScopeContext* _Cur;
  vector<ScopeContext*> _History;
};

std::list<ScopeInfoSave> _bak_list;

ScopeContext*& Sema::GetCurScope() {
  return this->_cur_scope;
}

ScopeContext* Sema::GetScopeOf(ASTPointer ast) {
  return this->_scope_context->find_child_scope(ast);
}

ScopeContext* Sema::EnterScope(ASTPointer ast) {
  auto scope = this->GetCurScope()->find_child_scope(ast);

  assert(scope);

  this->GetCurScope() = this->_scope_history.emplace_back(scope);

  return scope;
}

ScopeContext* Sema::EnterScope(ScopeContext* ctx) {
  auto scope = this->GetCurScope()->find_child_scope(ctx);

  assert(scope);

  this->GetCurScope() = this->_scope_history.emplace_back(scope);

  return scope;
}

void Sema::LeaveScope() {
  alert;
  this->_scope_history.pop_back();

  alert;
  this->GetCurScope() = *this->_scope_history.rbegin();
}

/*
void Sema::LeaveScope(ASTPointer ast) {
  // assert(this->GetCurScope()->GetAST() == ast);

  alert;
  this->_scope_history.pop_back();

  alert;
  this->GetCurScope() = *this->_scope_history.rbegin();
}

void Sema::LeaveScope(ScopeContext* ctx) {
  assert(this->GetCurScope() == ctx);

  alert;
  this->_scope_history.pop_back();

  alert;
  this->GetCurScope() = *this->_scope_history.rbegin();
}
*/

void Sema::SaveScopeInfo() {
  alert;
  _bak_list.push_front(
      ScopeInfoSave{._Cur = this->_cur_scope, ._History = this->_scope_history});
}

void Sema::RestoreScopeInfo() {
  alert;

  auto& save = *_bak_list.begin();

  this->_cur_scope = save._Cur;
  this->_scope_history = std::move(save._History);

  _bak_list.pop_front();
}

void Sema::BackToDepth(int depth) {
  alert;

  while (this->GetCurScope()->depth != depth)
    this->LeaveScope();
}

int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth) {
  alert;

  if (scope->depth == depth) {
    out.emplace_back(scope);
  }

  switch (scope->type) {
  case ScopeContext::SC_Block: {
    auto block = (BlockScope*)scope;

    for (auto&& child : block->child_scopes) {
      GetScopesOfDepth(out, child, depth);
    }

    break;
  }

  case ScopeContext::SC_Func: {
    GetScopesOfDepth(out, ((FunctionScope*)scope)->block, depth);
    break;
  }

  default:
    todo_impl;
  }

  return (int)out.size();
}

ScopeContext::LocalVar* Sema::_find_variable(string const& name) {

  for (auto it = this->_scope_history.rbegin(); it != this->_scope_history.rend(); it++) {
    auto lvar = (*it)->find_var(name);

    if (lvar)
      return lvar;
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

} // namespace fire::semantics_checker