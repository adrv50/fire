#include "alert.h"

#include "TypeInfo.h"
#include "Object.h"
#include "Token.h"
#include "Node.h"

#include "Evaluator.h"

Obj obj_add(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(lhs->as_int()->val + rhs->as_int()->val);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(lhs->as_float()->val + rhs->as_float()->val);

  if (lhs->ti.is(TypeKind::String))
    return ObjStr::make(lhs->as_str()->val + rhs->as_str()->val);

  todo_impl;
}

Obj obj_sub(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(lhs->as_int()->val - rhs->as_int()->val);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(lhs->as_float()->val - rhs->as_float()->val);

  todo_impl;
}

Obj obj_mul(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(lhs->as_int()->val * rhs->as_int()->val);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(lhs->as_float()->val * rhs->as_float()->val);

  todo_impl;
}

Obj obj_div(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(lhs->as_int()->val / rhs->as_int()->val);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(lhs->as_float()->val / rhs->as_float()->val);

  todo_impl;
}

Obj obj_mod(Obj lhs, Obj rhs) {
  debug(assert(lhs->ti.is(TypeKind::Int)));

  return ObjInt::make(lhs->as_int()->val % rhs->as_int()->val);
}

Obj obj_lshift(Obj lhs, Obj rhs) {
  return ObjInt::make(lhs->as_int()->val << rhs->as_int()->val);
}

Obj obj_rshift(Obj lhs, Obj rhs) {
  return ObjInt::make(lhs->as_int()->val >> rhs->as_int()->val);
}

Obj obj_compare(CompareExprKind kind, Obj lhs, Obj rhs) {
  switch (kind) {
    case CompareExprKind::CMP_Bigger:
      return ObjBool::make(lhs->as_int()->val > rhs->as_int()->val);

    case CompareExprKind::CMP_BiggerOrEqual:
      return ObjBool::make(lhs->as_int()->val >= rhs->as_int()->val);
  }

  return nullptr;
}

Obj obj_equal(Obj lhs, Obj rhs) {
  return ObjBool::make(lhs->equals(rhs));
}

Obj obj_bitand(Obj lhs, Obj rhs) {
  return ObjInt::make(lhs->as_int()->val & rhs->as_int()->val);
}

Obj obj_bitor(Obj lhs, Obj rhs) {
  return ObjInt::make(lhs->as_int()->val | rhs->as_int()->val);
}

Obj obj_bitxor(Obj lhs, Obj rhs) {
  return ObjInt::make(lhs->as_int()->val ^ rhs->as_int()->val);
}

Obj obj_or(Obj lhs, Obj rhs) {
  return ObjBool::make(lhs->as_bool()->val || rhs->as_bool()->val);
}

Obj obj_and(Obj lhs, Obj rhs) {
  return ObjBool::make(lhs->as_bool()->val && rhs->as_bool()->val);
}

//
// ND_Compare => Call directly
//
static const pair<NodeKind, Obj (*)(Obj, Obj)> ndkind_opfunc_table[] = {
    {ND_Mul, &obj_mul},       {ND_Div, &obj_div},     {ND_Mod, &obj_mod},
    {ND_Add, &obj_add},       {ND_Sub, &obj_sub},     {ND_LShift, &obj_lshift},
    {ND_RShift, &obj_rshift}, {ND_Compare, nullptr},  {ND_Equal, &obj_equal},
    {ND_BitAnd, &obj_bitand}, {ND_BitOr, &obj_bitor}, {ND_BitXor, &obj_bitxor},
    {ND_Or, &obj_or},         {ND_And, &obj_and},
};

// -----------------
//  Evaluator::CallStack::get
// ----------------------------------
Obj& Evaluator::CallStack::get(size_t index) {
  return this->objects[index];
}

// -----------------
//  Evaluator::CallStack::append
// ----------------------------------
Obj& Evaluator::CallStack::append(Obj obj) {
  return this->objects.emplace_back(obj);
}

// -----------------
//  Evaluator::CallStack::CallStack
// ----------------------------------
Evaluator::CallStack::CallStack() {
}

// -----------------
//  Evaluator::CallStack::~CallStack
// ----------------------------------
Evaluator::CallStack::~CallStack() {
  this->objects.clear();
}

// -----------------
//  Evaluator::get_current_call_stack
// ----------------------------------
Evaluator::CallStack& Evaluator::get_current_call_stack() {
  return this->call_stack.back();
}

// -----------------
//  Evaluator::push
// ----------------------------------
Obj& Evaluator::push(Obj obj) {
  return this->get_current_call_stack().append(obj);
}

// -----------------
//  Evaluator::pop
// ----------------------------------
Obj Evaluator::pop() {
  auto obj = this->get_current_call_stack().objects.back();

  this->get_current_call_stack().objects.pop_back();

  return obj;
}

// -----------------
//  Evaluator::push_stack
// ----------------------------------
Evaluator::CallStack& Evaluator::push_stack() {
  return this->call_stack.emplace_back();
}

// -----------------
//  Evaluator::pop_stack
// ----------------------------------
void Evaluator::pop_stack() {
  this->call_stack.pop_back();
}

// -----------------
//  Evaluator::Evaluator
// ----------------------------------
Evaluator::Evaluator(Node* program)
    : program(program) {
  if (!program)
    return;

  for (auto&& item : this->program->nd_program_items) {
    if (item->is(ND_Let)) {
      this->global_variables.emplace_back(this->eval_expr(item->nd_let_init));
    }
  }
}

// -----------------
//  Evaluator::append_global_var
// ----------------------------------
Obj& Evaluator::append_global_var(Obj obj) {
  return this->global_variables.emplace_back(obj);
}

// -----------------
//  Evaluator::evaluate
// ----------------------------------
Obj Evaluator::evaluate() {
  auto& main_stack = this->push_stack();

  (void)main_stack;
  // todo append [argc, argv] to main_stack

  Obj result = nullptr;

  for (auto&& item : this->program->nd_program_main->nd_func_body->nd_elements) {
    result = this->eval_stmt(item);
  }

  this->pop_stack();

  return result;
}

// -----------------
//  Evaluator::eval_expr
// ----------------------------------
Obj Evaluator::eval_expr(Node* node) {

  switch (node->kind) {

    case ND_Value:
      return node->nd_value;

    case ND_Array: {
      auto obj = ObjVector::make({});

      for (auto&& item : node->nd_array_elements)
        obj->list.emplace_back(this->eval_expr(item));

      if (!obj->list.empty())
        obj->ti.tp_args[0] = obj->list[0]->ti;

      return obj;
    }

    case ND_Tuple: {
      auto obj = ObjTuple::make({});

      for (auto&& item : node->nd_tuple_elements) {
        auto elem = this->eval_expr(item);

        obj->ti.append_template_arg(elem->ti);

        obj->list.emplace_back(elem);
      }

      return obj;
    }

    case ND_Variable:
      if (node->nd_variable_is_global)
        return this->global_variables[node->nd_variable_offset];

      return this->get_current_call_stack().get(node->nd_variable_offset);

    //
    // Call function
    //
    case ND_CallFunc: {
      Vec<Obj> args;

      for (auto&& item : node->nd_callfunc_args)
        args.emplace_back(this->eval_expr(item));

      return this->eval_call_func(node, args);
    }

    //
    // Contruct enumerator with intializers
    //
    case ND_ConstructEnumeratorValue: {
      auto obj = ObjEnumerator::make(node->nd_callfunc_enum_ctor_enum,
                                     node->nd_callfunc_enum_ctor_index);

      for (auto&& arg : node->nd_callfunc_args)
        obj->data.emplace_back(this->eval_expr(arg));

      return obj;
    }

    default:
      break;
  }

  auto lhs = this->eval_expr(node->nd_lhs)->clone();
  auto rhs = this->eval_expr(node->nd_rhs);

  if (node->kind == ND_Compare)
    return obj_compare(node->cmp_kind, lhs, rhs);

  return ndkind_opfunc_table[static_cast<size_t>(node->kind - ND_Mul)].second(lhs, rhs);
}

// -----------------
//  Evaluator::eval_call_func
// ----------------------------------
Obj Evaluator::eval_call_func(Node* node, Vec<Obj>& args) {
  auto callee = node->nd_callfunc_callee_userdef;

  if (!callee) {
    // => no pointer to user-defined function, call builtin

    assert(node->nd_callfunc_callee_builtin);

    return node->nd_callfunc_callee_builtin->call(*this, node, args);
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

// -----------------
//  Evaluator::eval_stmt
// ----------------------------------
Obj Evaluator::eval_stmt(Node* node) {
  switch (node->kind) {

    //
    // Block
    //
    case ND_Block:
      this->eval_block(node);
      break;

    //
    // Let
    //
    case ND_Let:
      this->eval_let(node);
      break;

    //
    // If
    //
    case ND_If: {
      auto cond = this->eval_expr(node->nd_if_cond);

      if (cond->as_bool()->val)
        this->eval_block(node->nd_if_then);
      else if (node->nd_if_else)
        this->eval_block(node->nd_if_else);

      break;
    }

    //
    // Return
    //
    case ND_Return: {
      auto& stack = this->get_current_call_stack();

      stack.pass = true;

      if (node->nd_return_expr)
        stack.result = this->eval_expr(node->nd_return_expr);

      break;
    }

    //
    // Expression statement
    //
    default:
      return this->eval_expr(node);
  }

  return nullptr;
}

// -----------------
//  Evaluator::eval_block
// ----------------------------------
Obj Evaluator::eval_block(Node* node) {
  Obj result = nullptr;

  for (auto&& item : node->nd_elements) {
    result = this->eval_stmt(item);

    if (this->get_current_call_stack().pass)
      break;
  }

  return result;
}

// -----------------
//  Evaluator::eval_let
// ----------------------------------
void Evaluator::eval_let(Node* node) {
  Obj val = nullptr;

  if (auto const x = node->nd_let_init)
    val = this->eval_expr(x);
  else
    val = Object::none;

  this->get_current_call_stack().append(val);
}
