#include <list>

#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

#include "Builtin.h"

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

std::vector<std::list<ScopeContext*>> _bak_list;

TypeInfo Sema::eval_type_name(ASTPtr<AST::TypeName> ast) {
  auto& name = ast->GetName();

  TypeInfo type = TypeInfo::from_name(name);

  switch (type.kind) {
  case TypeKind::Function: {
    todo_impl;
    // add signature

    break;
  }

  case TypeKind::Unknown:
    break;

  default: {
    for (auto&& p : ast->type_params)
      type.params.emplace_back(this->eval_type_name(p));

    if (auto c = type.needed_param_count(); c == 0 && type.params.size() >= 1) {
      throw Error(ast->token, "'" + ast->GetName() + "' type is not a template");
    }
    else if (1 <= c && c > (int)type.params.size()) {
      throw Error(ast->token, "too few template arguments");
    }
    else if (1 <= c && c < (int)type.params.size()) {
      throw Error(ast->token, "too many template arguments");
    }

    return type;
  }
  }

  if (auto tp = this->find_template_parameter_name(name); tp)
    return *tp;

  auto rs = this->find_name(name);

  switch (rs.type) {
  case NameType::Enum: {
    TypeInfo ret = TypeKind::Enumerator;

    ret.type_ast = rs.ast_enum;

    return ret;
  }

  case NameType::Class:
    return TypeInfo::make_instance_type(rs.ast_class);
  }

  throw Error(ast->token, "unknown type name");
}

ScopeContext* Sema::GetRootScope() {
  return this->_scope_context;
}

ScopeContext*& Sema::GetCurScope() {
  assert(this->_scope_history.size() >= 1);

  return *this->_scope_history.begin();
}

ScopeContext* Sema::GetScopeOf(ASTPointer ast) {
  return this->_scope_context->find_child_scope(ast);
}

ScopeContext* Sema::EnterScope(ASTPointer ast) {

  auto& cur = this->GetCurScope();

  // auto scope = cur->find_child_scope(ast);

  auto scope = ast->scope_ctx_ptr;

  if (!scope && ast->kind == ASTKind::Namespace && cur->is_block) {
    alert;

    for (auto&& c : ((BlockScope*)cur)->child_scopes) {
      if (c->type != ScopeContext::SC_Namespace)
        continue;

      auto ns = (NamespaceScope*)c;

      if (std::find(ns->_ast.begin(), ns->_ast.end(), ast) != ns->_ast.end()) {
        scope = c;
        break;
      }
    }
  }

  if (!scope) {
    Error(ast->token, "scope is nullptr").emit();

    panic;
  }

  return this->EnterScope(scope);
}

ScopeContext* Sema::EnterScope(ScopeContext* ctx) {
  // debug(assert(this->GetCurScope()->contains(ctx)));

  return this->_scope_history.emplace_front(ctx);
}

void Sema::LeaveScope() {
  this->_scope_history.pop_front();
}

void Sema::SaveScopeLocation() {

  _bak_list.emplace_back(this->_scope_history);
}

void Sema::RestoreScopeLocation() {
  this->_scope_history = *_bak_list.rbegin();

  _bak_list.pop_back();
}

void Sema::BackToDepth(int depth) {
  while (this->GetCurScope()->depth != depth)
    this->LeaveScope();
}

bool Sema::BackTo(ScopeContext* ctx) {
  while (this->_scope_history.size() >= 1) {
    if (this->GetCurScope() == ctx) {
      return true;
    }

    this->LeaveScope();
  }

  return false;
}

int GetScopesOfDepth(vector<ScopeContext*>& out, ScopeContext* scope, int depth) {
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

ScopeContext::LocalVar* Sema::_find_variable(string_view const& name) {

  for (auto&& scope : this->GetHistory()) {
    auto lvar = scope->find_var(name);

    if (lvar)
      return lvar;
  }

  return nullptr;
}

ASTVec<Function> Sema::_find_func(string_view const& name) {
  ASTVec<AST::Function> v;

  for (auto&& scope : this->GetHistory()) {
    if (!scope->is_block)
      continue;

    for (auto&& e : ((BlockScope*)scope)->ast->list) {
      if (auto f = ASTCast<AST::Function>(e);
          f->kind == ASTKind::Function && f->GetName() == name)
        v.emplace_back(f);
    }
  }

  return v;
}

ASTPtr<Enum> Sema::_find_enum(string_view const& name) {
  return ASTCast<AST::Enum>(this->context_reverse_search([&name](ASTPointer p) {
    return p->kind == ASTKind::Enum && p->As<AST::Named>()->GetName() == name;
  }));
}

ASTPtr<Class> Sema::_find_class(string_view const& name) {
  return ASTCast<AST::Class>(this->context_reverse_search([&name](ASTPointer p) {
    return p->kind == ASTKind::Class && p->As<AST::Named>()->GetName() == name;
  }));
}

ASTPtr<Block> Sema::_find_namespace(string_view const& name) {
  return ASTCast<AST::Block>(this->context_reverse_search([&name](ASTPointer p) {
    return p->kind == ASTKind::Namespace && p->token.str == name;
  }));
}

ASTPointer Sema::context_reverse_search(std::function<bool(ASTPointer)> func) {
  for (auto&& scope : this->GetHistory()) {
    if (!scope->is_block)
      continue;

    for (auto&& e : ((BlockScope*)scope)->ast->list) {
      if (func(e))
        return e;
    }
  }

  return nullptr;
}

Sema::NameFindResult Sema::find_name(string_view const& name, bool const only_cur_scope) {

  NameFindResult result = {};

  if (only_cur_scope) {
    this->SaveScopeLocation();
    this->GetHistory() = {this->GetCurScope()};
  }

  result.lvar = this->_find_variable(name);

  if (result.lvar) {
    result.type = NameType::Var;

    if (only_cur_scope) {
      this->RestoreScopeLocation();
    }

    return result;
  }

  result.functions = this->_find_func(name);

  if (result.functions.size() >= 1)
    result.type = NameType::Func;

  else if ((result.ast_enum = this->_find_enum(name)))
    result.type = NameType::Enum;

  else if ((result.ast_class = this->_find_class(name)))
    result.type = NameType::Class;

  else if ((result.ast_namespace = this->_find_namespace(name)))
    result.type = NameType::Namespace;

  else {
    // find builtin func
    for (builtins::Function const& bfunc : builtins::get_builtin_functions()) {
      if (bfunc.name == name) {
        result.builtin_funcs.emplace_back(&bfunc);
      }
    }
  }

  if (result.builtin_funcs.size() >= 1) {
    result.type = NameType::BuiltinFunc;
  }

  else if ((result.kind = TypeInfo::from_name(name)) != TypeKind::Unknown) {
    result.type = NameType::TypeName;
  }

  if (only_cur_scope) {
    this->RestoreScopeLocation();
  }

  return result;
}

string Sema::IdentifierInfo::to_string() const {
  string s = string(this->ast->GetName());

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

Sema::IdentifierInfo Sema::get_identifier_info(ASTPtr<AST::Identifier> ast,
                                               bool only_cur_scope) {
  IdentifierInfo id_info{};

  id_info.ast = ast;
  id_info.result = this->find_name(ast->GetName(), only_cur_scope);

  for (auto&& x : ast->id_params) {
    assert(x->is_ident_or_scoperesol());

    x->GetID()->sema_must_completed = false;

    auto& y = id_info.id_params.emplace_back(this->eval_type(x));

    if (y.kind == TypeKind::TypeName && y.params.size() == 1) {
      y = y.params[0];
    }

    if (!id_info.template_args.emplace_back(this->get_identifier_info(x))
             .result.is_type_name()) {
      throw Error(x, "expected type-name expression");
    }
  }

  ast->template_args = id_info.id_params;

  switch (id_info.result.type) {
  case NameType::Func:
  case NameType::MemberFunc: {
  }
  }

  return id_info;
}

// scope-resolution
Sema::IdentifierInfo Sema::get_identifier_info(ASTPtr<AST::ScopeResol> ast) {
  auto info = this->get_identifier_info(ast->first);

  // string idname = string(ast->first->GetName());
  string_view idname = ast->first->GetName();

  this->SaveScopeLocation();

  if (info.result.type == NameType::Namespace) {
    auto scope = this->GetScopeOf(info.result.ast_namespace);

    this->BackTo(scope->_owner);
  }

  for (auto&& id : ast->idlist) {
    string_view name = id->GetName();

    switch (info.result.type) {
    case NameType::Enum: {
      auto _enum = info.result.ast_enum;

      for (int _idx = 0; auto&& _e : _enum->enumerators) {
        if (_e.name.str == name) {
          info.ast = id;

          info.result.type = NameType::Enumerator;
          info.result.ast_enum = _enum;
          info.result.enumerator_index = _idx;
          info.result.name = name;

          goto _loop_continue;
        }

        _idx++;
      }

      throw Error(id->token, "enumerator '" + name + "' is not found in enum '" +
                                 _enum->GetName() + "'");
    }

    //
    // class
    //  => find static member function
    case NameType::Class: {
      auto _class = info.result.ast_class;

      for (auto&& mf : _class->member_functions) {
        if (mf->GetName() == name) {
          info.result.functions.emplace_back(mf);
        }
      }

      if (!id->sema_allow_ambiguous && info.result.functions.size() >= 2) {
        throw Error(id->token, idname + "::" + name + " is ambiguous");
      }

      if (info.result.functions.empty()) {
        throw Error(id->token, "member function '" + name + "' is not found in class '" +
                                   _class->GetName() + "'");
      }

      info.ast = id;
      info.result.type = NameType::MemberFunc;

      break;
    }

    case NameType::Namespace: {

      this->EnterScope(info.result.ast_namespace);

      info = this->get_identifier_info(id, true);

      break;
    }

    default:
      throw Error(info.ast, "'" + idname + "' is not enum or class or namespace");
    }

  _loop_continue:;
    idname = idname.substr(0, idname.length() + name.length() + 2);
    // idname += "::" + name;
  }

  this->RestoreScopeLocation();

  return info;
}

bool Sema::IsWritable(ASTPointer ast) {

  switch (ast->kind) {
  case ASTKind::Variable:
    return true;

  case ASTKind::IndexRef:
  case ASTKind::MemberAccess:
    return this->IsWritable(ASTCast<AST::Expr>(ast)->lhs);
  }

  return false;
}

TypeInfo Sema::ExpectType(TypeInfo const& type, ASTPointer ast) {
  this->_expected.emplace_back(type);

  if (auto t = this->eval_type(ast); !t.equals(type)) {
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

TypeInfo Sema::make_functor_type(ASTPtr<AST::Function> ast) {
  TypeInfo ret = TypeKind::Function;

  ret.params.emplace_back(this->eval_type(ast->return_type));

  for (ASTPtr<AST::Argument> const& arg : ast->arguments)
    ret.params.emplace_back(this->eval_type(arg->type));

  return ret;
}

TypeInfo Sema::make_functor_type(builtins::Function const* builtin) {
  TypeInfo ret = TypeKind::Function;

  ret.params = builtin->arg_types;
  ret.params.insert(ret.params.begin(), builtin->result_type);

  return ret;
}

} // namespace fire::semantics_checker