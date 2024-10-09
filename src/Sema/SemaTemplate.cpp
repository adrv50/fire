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

Error Sema::TemplateTypeApplier::make_error(ASTPointer ast) {
  (void)ast;

  todo_impl;

  switch (this->result) {
  case Result::TooManyParams:
    return Error(this->ast, "too many template arguments to instantiation of '" +
                                this->ast->GetName() + "'");

  case Result::CannotDeductType: {
    string s;

    for (auto&& t : this->ast->template_param_names)
      if (!this->find_parameter(t.str))
        s += t.str + ", ";

    s[s.length() - 2] = 0;

    return Error(this->ast, "cannot deduction type of parameter: [" + s + "]");
  }

  case Result::ArgumentError:
    switch (this->arg_result.result) {
    case ArgumentCheckResult::TypeMismatch:
      todo_impl;

    case ArgumentCheckResult::TooFewArguments:
    case ArgumentCheckResult::TooManyArguments:
      todo_impl;
    }
  }

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
    apply.add_parameter(ast->template_param_names[i].str, args[i]);
  }

  apply.result = TemplateTypeApplier::Result::NotChecked;

  return apply;
}

bool Sema::try_apply_template_function(TemplateTypeApplier& out,
                                       ASTPtr<AST::Function> ast, TypeVec const& args,
                                       TypeVec const& func_args) {
  if (ast->template_param_names.size() < args.size()) {
    out.result = TemplateTypeApplier::Result::TooManyParams;
    return false;
  }

  out = this->apply_template_params(ast, args);

  for (size_t i = 0; i < func_args.size(); i++) {
    auto const& name = ast->arguments[i]->type->GetName();

    auto find = out.find_parameter(name);

    if (find) {
      if (!find->type.equals(func_args[i])) {
        out.result = TemplateTypeApplier::Result::ArgumentError;
        out.arg_result = {ArgumentCheckResult::TypeMismatch, static_cast<int>(i)};

        return false;
      }
    }
    else {
      out.add_parameter(name, func_args[i]);
    }
  }

  for (size_t i = 0; auto&& pname : ast->template_param_names) {
    if (!out.find_parameter(pname.str)) {
      out.result = TemplateTypeApplier::Result::CannotDeductType;
      out.err_param_index = i;
      return false;
    }

    i++;
  }

  out.S = this;
  this->_applied_ptr_stack.push_front(&out);

  if (!this->_find_applied(out)) {
    debug(auto ii = this->_applied_ptr_stack.size());

    this->_applied_templates.emplace_back(out).S = nullptr;

    ast->is_templated = false;

    this->SaveScopeLocation();

    auto scope = this->GetScopeOf(ast);

    this->BackToDepth(scope->_owner->depth);

    this->check(ast);

    this->RestoreScopeLocation();

    ast->is_templated = true;

    debug(assert(this->_applied_ptr_stack.size() == ii));
  }

  return true;
}

Sema::TemplateTypeApplier* Sema::_find_applied(TemplateTypeApplier const& t) {
  for (auto&& x : this->_applied_templates) {
    if (x.ast != t.ast || x.parameter_list.size() != t.parameter_list.size())
      continue;

    for (size_t i = 0; i < t.parameter_list.size(); i++) {
      if (!x.parameter_list[i].type.equals(t.parameter_list[i].type))
        goto __skip;
    }

    return &x;
  __skip:;
  }

  return nullptr;
}

} // namespace fire::semantics_checker