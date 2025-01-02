#include <cassert>

#include "alert.h"

#include "Token.h"
#include "Node.h"
#include "Builtins.h"

#include "Error.h"
#include "Sema.h"

Sema::VarInfo::VarInfo()
    : name(""),
      ti(TypeKind::None) {
}

Sema::VarInfo::VarInfo(string const& name, TypeInfo const& ti)
    : name(name),
      ti(ti) {
}

//
// VarList::operator[]:
//   get variable by index.
//
Sema::VarInfo& Sema::VarList::operator[](size_t index) {
  return this->variables[index];
}

//
// VarList::append:
//   append variable to list.
//
Sema::VarInfo& Sema::VarList::append(VarInfo const& var) {
  auto size = this->variables.size();

  auto& emplaced = this->variables.emplace_back(var);

  emplaced.index = size;

  return emplaced;
}

//
// VarList::find:
//   find variable by name.
//
Sema::VarInfo* Sema::VarList::find(string const& name) {
  for (auto&& var : this->variables)
    if (var.name == name)
      return &var;

  return nullptr;
}

Sema::VarList::VarList()
    : variables() {
}

Sema::VarList::VarList(Vec<VarInfo> const& variables)
    : variables(variables) {
}

//
// Scope::get_name:
//   get name of scope.
//
string Sema::Scope::get_name() const {
  switch (this->type) {
  case SC_Function:
    return this->node->nd_func_name->str;

  case SC_Enum:
    return this->node->nd_enum_name->str;

  case SC_Class:
    return this->node->nd_class_name->str;

  case SC_Struct:
    return this->node->nd_struct_name->str;
  }

  return "";
}

//
// Scope::find_var:
//   find variable by name.
//
Sema::VarInfo* Sema::Scope::find_var(string const& name) {
  for (auto& var : this->variables)
    if (var.name == name)
      return &var;

  return nullptr;
}

//
// Scope::find_func:
//   find function by name.
//
Sema::Scope* Sema::Scope::find_func(string const& name) {
  for (auto& func : this->functions)
    if (func->get_name() == name)
      return func;

  return nullptr;
}

Sema::Scope* Sema::Scope::make_scope(Node* node) {
  switch (node->kind) {
  case ND_Program:
    break;

  case ND_Function: {
    auto scope = new Scope(SC_Function, node);

    scope->node = node;

    return scope;
  }

  case ND_Enum:
    todo_impl;
    break;

  case ND_Struct:
    todo_impl;
    break;

  case ND_Class:
    todo_impl;
    break;
  }

  auto scope = new Scope(SC_Global, node);

  for (auto&& item : node->nd_items) {
    if (item->is(ND_Function)) {
      auto fnscope = Scope::make_scope(item);

      scope->append(fnscope);
      scope->functions.emplace_back(fnscope);
    }
  }

  return scope;
}

Sema::Scope::Scope(ScopeType type, Node* node)
    : type(type),
      node(node) {
}
