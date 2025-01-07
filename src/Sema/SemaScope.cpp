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
size_t Sema::Scope::find_func(Sema* S, Vec<TypeInfo>* template_args,
                              Vec<TypeInfo>* arg_types, Vec<Scope*>& out,
                              string const& name) {
  for (auto& func : this->functions) {
    if (func->get_name() == name) {

      auto fn = func->node;

      if (fn->nd_func_is_template) {
        // instantiate!!!

        auto record = S->is_recorded_template_instantiation_pattern(fn, *template_args);

        if (record) {
          todo_impl;
        }

        auto tis = S->enter_template(fn);

        for (auto&& param_id : fn->nd_func_tplist->list) {
          tis->parameters.emplace_back(TemplateParameterInfo{.name = param_id->tok->str});
        }

        for (size_t i = 0; i < template_args->size(); i++) {
          auto& pi = tis->parameters[i];

          pi.type = template_args->operator[](i);
          pi.is_deducted = true;
        }

        auto ctx_keep = S->ctx;

        S->ctx.cur_scope = func->parent;

        S->ctx.cur_func = func;

        S->ctx.is_in_func = true;

        fn->nd_func_is_template = false;
        S->check_func(fn);
        fn->nd_func_is_template = true;

        alertmsg(S->eval_type_ti(fn->nd_func_result_type).to_string());

        S->ctx = ctx_keep;

        S->leave_template();

        todo_impl;
      }
      else if (template_args->size() >= 1) {
        todo_impl;
        // func 'name' is not template
      }

      out.push_back(func);
    }
  }

  return out.size();
}

Sema::Scope* Sema::Scope::find_if(std::function<bool(Scope*)> const& pred,
                                  bool recursive) {
  for (auto& child : this->childs) {
    if (pred(child))
      return child;

    if (recursive)
      if (auto found = child->find_if(pred, recursive))
        return found;
  }

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

    case ND_Enum: {
      return new Scope(SC_Enum, node);
    }

    case ND_Struct:
      todo_impl;
      break;

    case ND_Class:
      todo_impl;
      break;

    default:
      return nullptr;
  }

  auto scope = new Scope(SC_Global, node);

  for (auto&& item : node->nd_items) {
    if (auto s = Scope::make_scope(item)) {
      if (s->type == SC_Function)
        scope->append_func(s);
      else
        scope->append(s);
    }
  }

  return scope;
}

Sema::Scope::Scope(ScopeType type, Node* node)
    : type(type),
      node(node) {
}
