#include "alert.h"
#include "Evaluator.h"

Obj& Evaluator::CallStack::get(size_t index) {
  return this->objects[index];
}

Obj& Evaluator::CallStack::append(Obj obj) {
  return this->objects.emplace_back(obj);
}

Evaluator::CallStack::CallStack()
    : objects(),
      result(nullptr),
      is_returned(false) {
}

Evaluator::CallStack::~CallStack() {
  this->objects.clear();
}

Evaluator::CallStack& Evaluator::get_current_call_stack() {
  return this->call_stack.back();
}

Obj& Evaluator::push(Obj obj) {
  return this->get_current_call_stack().append(obj);
}

Obj Evaluator::pop() {
  auto obj = this->get_current_call_stack().objects.back();

  this->get_current_call_stack().objects.pop_back();

  return obj;
}

Evaluator::CallStack& Evaluator::push_stack() {
  return this->call_stack.emplace_back();
}

void Evaluator::pop_stack() {
  this->call_stack.pop_back();
}

Evaluator::Evaluator(Node* program)
    : program(program) {
  for (auto&& item : this->program->nd_program_items) {
    if (item->is(ND_Let)) {
      this->global_variables.emplace_back(this->eval_expr(item->nd_let_init));
    }
  }
}

Obj Evaluator::evaluate() {
  auto& main_stack = this->push_stack();

  // todo append [argc, argv] to main_stack

  for (auto&& item : this->program->nd_program_main->nd_func_body->nd_elements)
    this->eval_stmt(item);

  this->pop_stack();

  return main_stack.result;
}

Obj Evaluator::eval_expr(Node* node) {

  switch (node->kind) {

  case ND_Value:
    return node->nd_value;

  case ND_Identifier:
  case ND_ScopeResol:
    switch (node->id_kind) {
    case NodeIdentifierKind::ID_Var:
      if (node->nd_variable_is_global)
        return this->global_variables[node->nd_variable_offset];

      return this->get_current_call_stack().get(node->nd_variable_offset);

    case NodeIdentifierKind::ID_Func:
      todo_impl;

    default:
      todo_impl;
    }

    break;

  case ND_CallFunc: {
    Vec<Obj> args;

    if (node->nd_callfunc_is_method_call) {
      args.emplace_back(this->eval_expr(node->nd_callfunc_method_self));
    }

    for (auto&& item : node->nd_callfunc_args)
      args.emplace_back(this->eval_expr(item));

    return this->eval_call_func(node, args);
  }

  default:
    break;
  }

  auto lhs = this->eval_expr(node->nd_lhs)->clone();
  auto rhs = this->eval_expr(node->nd_rhs);

  switch (node->kind) {
  case ND_Add:
    switch (node->nd.tk) {
    case TypeKind::Int:
      lhs->as_int()->val += rhs->as_int()->val;
      break;

    case TypeKind::Float:
      lhs->as_float()->val += rhs->as_float()->val;
      break;

    default:
      todo_impl;
    }

  default:
    break;
  }

  return lhs;
}

Obj Evaluator::eval_call_func(Node* node, Vec<Obj>& args) {
  auto callee = node->nd_callfunc_callee_userdef;

  if (!callee) {
    // => no pointer to user-defined function, call builtin

    return node->nd_callfunc_callee_builtin->call(*this, args);
  }

  // if got pointer, call user-defined function
  auto& stack = this->push_stack();

  for (auto&& arg : args)
    this->push(arg);

  this->eval_block(callee->nd_func_body);

  auto result = stack.result;

  this->pop_stack();

  return result;
}

void Evaluator::eval_stmt(Node* node) {
  switch (node->kind) {

  case ND_Block:
    this->eval_block(node);
    break;

  case ND_Let:
    this->eval_let(node);
    break;

  case ND_Return: {
    auto& stack = this->get_current_call_stack();

    stack.is_returned = true;

    if (node->nd_return_expr)
      stack.result = this->eval_expr(node->nd_return_expr);

    break;
  }

  default:
    this->eval_expr(node);
    break;
  }
}

void Evaluator::eval_block(Node* node) {
  for (auto&& item : node->nd_elements) {
    this->eval_stmt(item);

    if (this->get_current_call_stack().is_returned)
      break;
  }
}

void Evaluator::eval_let(Node* node) {
  Obj val = nullptr;

  if (node->nd_let_init)
    val = this->eval_expr(node->nd_let_init);
  else
    val = Object::none;

  this->get_current_call_stack().append(val);
}
