#include <list>

#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

#include "Builtin.h"

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

std::list<Sema::ScopeLocation> _bak_list;

TypeInfo Sema::eval_type_name(ASTPtr<AST::TypeName> ast) {
  auto& name = ast->GetName();

  TypeInfo type = TypeInfo::from_name(name);

  switch (type.kind) {
  case TypeKind::Function: {
  }

  case TypeKind::Unknown:
    break;

  default:
    return type;
  }

  if (auto tp = this->find_template_parameter_name(name); tp)
    return *tp;

  auto rs = this->find_name(name);

  switch (rs.type) {
  case NameType::Enum: {
    TypeInfo ret = TypeInfo::from_enum(rs.ast_enum);

    ret.kind = TypeKind::Enumerator;

    return ret;
  }

  case NameType::Class:
    return TypeInfo::from_class(rs.ast_class);
  }

  throw Error(ast->token, "unknown type name");
}

ScopeContext* Sema::GetRootScope() {
  return this->_scope_context;
}

ScopeContext*& Sema::GetCurScope() {
  return this->_location.Current;
}

ScopeContext* Sema::GetScopeOf(ASTPointer ast) {
  return this->_scope_context->find_child_scope(ast);
}

ScopeContext* Sema::EnterScope(ASTPointer ast) {
  auto scope = this->GetCurScope()->find_child_scope(ast);

  this->GetCurScope() = this->_location.History.emplace_back(scope);

  return scope;
}

ScopeContext* Sema::EnterScope(ScopeContext* ctx) {
  auto scope = this->GetCurScope()->find_child_scope(ctx);

  this->GetCurScope() = this->_location.History.emplace_back(scope);

  return scope;
}

void Sema::LeaveScope() {
  this->_location.History.pop_back();

  this->GetCurScope() = *this->_location.History.rbegin();
}

void Sema::SaveScopeLocation() {
  _bak_list.push_front(ScopeLocation{.Current = this->_location.Current,
                                     .History = this->_location.History});
}

void Sema::RestoreScopeLocation() {
  auto& save = *_bak_list.begin();

  this->_location.Current = save.Current;
  this->_location.History = std::move(save.History);

  _bak_list.pop_front();
}

Sema::ScopeLocation Sema::GetScopeLoc() {
  return this->_location;
}

void Sema::BackToDepth(int depth) {
  while (this->GetCurScope()->depth != depth)
    this->LeaveScope();
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

ScopeContext::LocalVar* Sema::_find_variable(string const& name) {

  for (auto it = this->_location.History.rbegin(); it != this->_location.History.rend();
       it++) {
    auto lvar = (*it)->find_var(name);

    if (lvar)
      return lvar;
  }

  return nullptr;
}

ASTVec<Function> Sema::_find_func(string const& name) {
  ASTVec<Function> vec;

  auto&& found = this->GetRootScope()->find_name(name);

  for (auto&& _s : found) {
    if (_s->type == ScopeContext::SC_Func &&
        _s->depth <= this->GetCurScope()->depth + 1) {
      vec.emplace_back(ASTCast<AST::Function>(_s->GetAST()));
    }
  }

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
  else {
    // clang-format off
    static std::pair<TypeKind, char const*> const kind_name_map[] {
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

    for (auto&& [k, s] : kind_name_map) {
      if (s == name) {
        result.type = NameType::TypeName;
        result.kind = k;
        break;
      }
    }
  }

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

  for (auto&& x : ast->id_params) {
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

  string idname = ast->first->GetName();

  for (auto&& id : ast->idlist) {
    auto name = id->GetName();

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

      if (!id->sema_allow_ambigious && info.result.functions.size() >= 2) {
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

    default:
      throw Error(id->token, "'" + idname + "' is not enum or class");
    }

  _loop_continue:;
    idname += "::" + name;
  }

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