#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace fire::eval {

using namespace AST;

ObjPointer* Evaluator::find_var(std::string const& name) {
  for (auto it = this->stack.rbegin(); it != this->stack.rend();
       it++) {
    if (auto pvar = it->find_variable(name); pvar)
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

std::pair<ASTPtr<AST::Class>, ASTPtr<AST::Enum>>
Evaluator::find_class_or_enum(std::string const& name) {
  for (auto&& ast : this->classes) {
    if (ast->GetName() == name)
      return {ast, nullptr};
  }

  return {nullptr, this->find_enum(name)};
}

ASTPtr<AST::Enum> Evaluator::find_enum(std::string const& name) {
  for (auto&& e : this->enums)
    if (e->GetName() == name)
      return e;

  return nullptr;
}

ObjPtr<ObjInstance>
Evaluator::new_class_instance(ASTPtr<AST::Class> ast) {
  auto obj = ObjNew<ObjInstance>(ast);

  for (auto&& var : ast->get_member_variables()) {
    auto& v = obj->member[var->GetName()];

    if (var->init) {
      v = this->evaluate(var->init);
    }
  }

  for (auto&& func : ast->get_member_functions()) {
    obj->add_member_func(ObjNew<ObjCallable>(func));
  }

  return obj;
}

ObjPointer Evaluator::call_function_ast(bool have_self,
                                        ASTPtr<AST::Function> ast,
                                        ASTPtr<AST::CallFunc> call,
                                        ObjVector& args) {

  if (this->stack.size() >= EVALUATOR_STACK_MAX_SIZE)
    Error(call->token, "stack over flow.")();

  auto& stack = this->PushStack();

  std::map<std::string, bool> wasset;

  auto formal = ast->arg_names.begin();
  auto act = call->args.begin();

  if (have_self) {
    wasset["self"] = true;
    stack.append("self", args[0]);
    formal++;
  }

  alert;
  alertmsg("formal->str = " << formal->str);
  alertmsg("(*act)->token.str = " << (*act)->token.str);

  for (auto itobj = args.begin() + (int)have_self;
       act != call->args.end(); act++) {
    if (formal == ast->arg_names.end() && !ast->is_var_arg) {
      Error(call->token, "too many arguments")();
    }

    std::string name;

    if ((*act)->kind == ASTKind::SpecifyArgumentName) {
      name = (*act)->As<AST::Expr>()->lhs->token.str;
    }
    else {
      name = formal++->str;
    }

    if (wasset[name]) {
      Error(*act, "set to same argument name again.")();
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

  this->PopStack();

  return ret ? ret : ObjNew<ObjNone>();
}

Evaluator::LoopContext&
Evaluator::EnterLoopStatement(ASTPtr<AST::Statement> ast) {
  return this->loop_stack.emplace_back(ast);
}

void Evaluator::LeaveLoop() {
  this->loop_stack.pop_back();
}

Evaluator::LoopContext& Evaluator::GetCurrentLoop() {
  return *this->loop_stack.rbegin();
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
  else // rk == TypeKind::Size
    left->vsize = (value_type::Size)left->vi;
}

} // namespace fire::eval