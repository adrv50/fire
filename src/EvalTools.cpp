#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace metro::eval {

using namespace AST;

ObjPointer* Evaluator::find_var(std::string const& name) {
  for (i64 i = this->stack.size() - 1; i >= 0; i--) {
    auto& stack = this->stack[i];

    if (auto pvar = stack.find_variable(name); pvar)
      return &pvar->value;
  }

  return nullptr;
}

std::pair<ASTPtr<AST::Function>, builtins::Function const*>
Evaluator::find_func(std::string const& name) {
  for (auto&& ast : this->functions) {
    if (ast->GetName() == name)
      return {ast, nullptr};
  }

  return {nullptr, builtins::find_builtin_func(name)};
}

ASTPtr<AST::Class> Evaluator::find_class(std::string const& name) {
  for (auto&& ast : this->classes) {
    if (ast->GetName() == name)
      return ast;
  }

  return nullptr;
}

ObjPtr<ObjInstance>
Evaluator::new_class_instance(ASTPtr<AST::Class> ast) {
  auto obj = ObjNew<ObjInstance>(ast);

  for (auto&& var : ast->mb_variables) {
    auto& v = obj->member[var->GetName()];

    if (var->init) {
      v = this->evaluate(var->init);
    }
  }

  for (auto&& func : ast->mb_functions) {
    obj->add_member_func(ObjNew<ObjCallable>(func));
  }

  return obj;
}

ObjPointer Evaluator::call_function_ast(ASTPtr<AST::Function> ast,
                                        ASTPtr<AST::CallFunc> call,
                                        ObjVector& args) {

  auto& stack = this->push();

  std::map<std::string, bool> wasset;

  auto formal_iter = ast->arg_names.begin();
  auto actual_iter = call->args.begin();

  for (auto itobj = args.begin(); actual_iter != call->args.end();
       actual_iter++) {
    if (formal_iter == ast->arg_names.end() && !ast->is_var_arg) {
      Error(call->token, "too many arguments")();
    }

    std::string name;

    if ((*actual_iter)->kind == ASTKind::SpecifyArgumentName) {
      name = (*actual_iter)->As<AST::Expr>()->lhs->token.str;
    }
    else {
      name = formal_iter++->str;
    }

    if (wasset[name]) {
      Error(*actual_iter, "set to same argument name again.")();
    }

    wasset[name] = true;

    stack.append(name, *itobj++);
  }

  for (auto&& arg : ast->arg_names) {
    if (!wasset[arg.str]) {
      Error(call->token,
            "argument '" + arg.str + "' was not assignment")();
    }
  }

  this->evaluate(ast->block);

  auto ret = stack.result;
  this->pop();

  return ret ? ret : ObjNew<ObjNone>();
}

void Evaluator::adjust_numeric_type_object(
    ObjPtr<ObjPrimitive> left, ObjPtr<ObjPrimitive> right) {

  auto lk = left->type.kind, rk = right->type.kind;

  if (lk == rk)
    return;

  if (left->is_float() != right->is_float()) {
    (left->is_float() ? left : right)->to_float();
    return;
  }

  if (lk == TypeKind::Size)
    right->vsize = (value_type::Size)right->vi;
  else
    left->vsize = (value_type::Size)left->vi;
}

} // namespace metro::eval