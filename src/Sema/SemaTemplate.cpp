
#include "Error.h"
#include "Sema/Sema.h"

namespace fire::sema::templates {

void ParamList::subtitute(Node* id, Vec<TypeInfo> const& args,
                          Vec<TypeInfo> const* callfunc_args, Node* cf_expr) {

  for (size_t i = 0; i < args.size(); i++) {
    TypeInfo const& given_arg = args[i];

    if (i >= this->size()) {
      Error(id->nd_id_tp_args[i], "too many template arguments").crash();
    }

    Parameter& param = this->params[i];

    param.type = given_arg;
    param.is_deducted = true;
  }

  if (callfunc_args) {
    auto const& cf_args = *callfunc_args;

    for (size_t i = 0; i < cf_args.size(); i++) {
      TypeInfo const& arg = cf_args[i];
      Parameter* param = this->get_param_of_arg(i);

      if (!param)
        continue;

      if (param->is_deducted) {
        if (!param->type.equals(arg))
          Error(cf_expr->nd_callfunc_args[i], "expected '" + param->type.to_string() +
                                                  "' type expression, but found '" +
                                                  arg.to_string() + "'")
              .crash();
      }
      else {
        param->type = arg;
        param->is_deducted = true;
      }
    }
  }

  for (auto&& p : this->params) {
    if (!p.is_deducted)
      Error(id, "cannot deduct type of parameter '" + p.name + "'").crash();
  }
}

Parameter* ParamList::find(string const& name) {
  for (auto&& p : this->params)
    if (p.name == name)
      return &p;

  return nullptr;
}

Parameter* ParamList::get_param_of_arg(size_t index) {

  assert(this->parent->node->kind == ND_Function);

  auto const& args = this->parent->node->nd_func_args;

  if (index >= args.size())
    return nullptr;

  Node* arg = args[index];

  auto const& argtype_str = arg->nd_func_arg_type->nd_type_id->tok->str;

  if (auto p = this->find(argtype_str); p)
    return p;

  return nullptr;
}

ParamList::ParamList(DefinitionIR* parent)
    : parent(parent) {
}

TemplateManager::TemplateManager() {
}

DefinitionIR* TemplateManager::find_ir_from_sym(Symbol* sym) {
  for (auto&& ir : this->definitions)
    if (ir->sym == sym)
      return ir;

  return nullptr;
}

Instantiated* TemplateManager::find_instantiated(Symbol* sym,
                                                 Vec<TypeInfo> const& tp_args,
                                                 Vec<TypeInfo> const& cf_args) {
  auto ir = this->find_ir_from_sym(sym);

  for (auto&& inst : this->instantiations) {
    if (inst->based == ir) {

      if (inst->take_args.size() != tp_args.size())
        continue;

      if (inst->take_cf_args.size() != cf_args.size())
        continue;

      for (size_t i = 0; i < tp_args.size(); i++)
        if (!inst->take_args[i].equals(tp_args[i]))
          goto __pass;

      for (size_t i = 0; i < cf_args.size(); i++)
        if (!inst->take_cf_args[i].equals(cf_args[i]))
          goto __pass;

      return inst;
    }

  __pass:;
  }

  return nullptr;
}

Instantiated* TemplateManager::instantiate(DefinitionIR* ir, Node* id,
                                           Vec<TypeInfo> const& tp_args,
                                           Vec<TypeInfo> const& cf_args, Node* cf_expr) {
  if (auto inst = this->find_instantiated(ir->sym, tp_args, cf_args); inst)
    return inst;

  auto inst = this->instantiations.emplace_back(new Instantiated(ir));

  inst->take_args = tp_args;
  inst->take_cf_args = cf_args;

  inst->params.subtitute(id, tp_args, &cf_args, cf_expr);

  inst->node = this->replace_all_params(inst, ir->node);

  ir->instantiated_list.emplace_back(inst);

  return inst;
}

DefinitionIR* TemplateManager::add_define(Symbol* sym) {
  if (auto ir = this->find_ir_from_sym(sym); ir)
    return ir;

  auto ir = this->definitions.emplace_back(new DefinitionIR(sym));

  Node* tplist = nullptr;

  switch (auto nd = sym->decl; nd->kind) {
    case ND_Function:
      tplist = nd->nd_func_tplist;
      break;

    case ND_Class:
      tplist = nd->nd_class_tplist;
      break;

    default:
      todo_impl;
  }

  for (auto&& param : tplist->list) {
    auto& p = ir->param_list.push(Parameter(param));
  }

  return ir;
}

Node* TemplateManager::replace_all_params(Instantiated* inst, Node* _node) {
  Node* cloned = _node->clone();

  Node::walk_node(cloned, [&](Node* nd) -> bool {
    switch (nd->kind) {
      case ND_Identifier: {
        for (auto&& tp : nd->nd_id_tp_args) {
          if (tp->is(ND_Identifier)) {
            if (auto p = inst->params.find(tp->nd_id_name->str)) {
              tp->nd_id_name->str = p->name;
            }
          }
        }

        break;
      }

      case ND_TypeName:
        if (auto p = inst->params.find(nd->nd_type_id->nd_id_name->str); p) {
          alert;
          nd->nd_type_id->nd_id_name->str = p->type.to_string();
        }

        break;

      case ND_TemplateParameterList: {
        for (auto&& id : nd->list) {
          if (auto p = inst->params.find(id->nd_id_name->str)) {
            id->nd_id_name->str = p->name;
          }
        }

        break;
      }
    }

    return false;
  });

  return cloned;
}
} // namespace fire::sema::templates