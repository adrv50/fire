#include "Sema/Sema.h"

namespace fire::semantics_checker {

Sema::TemplateTypeApplier::Parameter&
Sema::TemplateTypeApplier::add_parameter(string_view name, TypeInfo type) {
  return this->parameter_list.emplace_back(name, std::move(type));
}

Sema::TemplateTypeApplier::Parameter*
Sema::TemplateTypeApplier::find_parameter(string_view name) {
  for (auto&& param : this->parameter_list)
    if (param.name == name)
      return &param;

  return nullptr;
}

Sema::TemplateTypeApplier::TemplateTypeApplier()
    : S(nullptr),
      ast(nullptr) {
}

Sema::TemplateTypeApplier::TemplateTypeApplier(ASTPtr<AST::Templatable> ast,
                                               vector<Parameter> parameter_list)
    : S(nullptr),
      ast(ast),
      parameter_list(std::move(parameter_list)) {
}

Sema::TemplateTypeApplier::~TemplateTypeApplier() {
  if (S) {
    alert;
    S->_applied_ptr_stack.pop_front();
  }
}

Error Sema::TemplateTypeApplier::make_error(ASTPointer) {

  panic;
}

TypeInfo* Sema::find_template_parameter_name(string_view const& name) {
  for (auto&& pt : this->_applied_ptr_stack) {
    if (auto p = pt->find_parameter(name); p)
      return &p->type;
  }

  return nullptr;
}

//
// args = template arguments (parameter)
//
Sema::TemplateTypeApplier Sema::apply_template_params(ASTPtr<AST::Templatable> ast,
                                                      TypeVec const& args) {
  TemplateTypeApplier apply{ast};

  for (size_t i = 0; i < args.size(); i++) {
    apply.add_parameter(ast->template_param_names[i].token.str, args[i]);
  }

  apply.result = TemplateTypeApplier::Result::NotChecked;

  return apply;
}

Sema::TemplateTypeApplier* Sema::_find_applied(TemplateTypeApplier const& t) {
  for (auto&& x : this->_applied_templates) {
    if (x.ast != t.ast)
      continue;

    if (x.parameter_list.size() != t.parameter_list.size())
      continue;

    for (size_t i = 0; i < t.parameter_list.size(); i++) {
      auto& A = x.parameter_list[i];
      auto& B = t.parameter_list[i];

      if (A.name != B.name || !A.type.equals(B.type)) {
        goto __skip;
      }
    }

    return &x;
  __skip:;
  }

  return nullptr;
}

} // namespace fire::semantics_checker