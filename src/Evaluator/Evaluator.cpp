#include "Debug/alert.h"

#include "TypeInfo.h"
#include "Object.h"
#include "Token/Token.h"
#include "Node/Node.h"

#include "Builtins.h"
#include "Evaluator.h"

#define Li lhs->as_int()->val
#define Ri rhs->as_int()->val
#define Lf lhs->as_float()->val
#define Rf rhs->as_float()->val

namespace fire {

Obj obj_add(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(Li + Ri);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(Lf + Rf);

  if (lhs->ti.is(TypeKind::String))
    return ObjStr::make(lhs->as_str()->val + rhs->as_str()->val);

  todo_impl;
}

Obj obj_sub(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(Li - Ri);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(Lf - Rf);

  todo_impl;
}

Obj obj_mul(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(Li * Ri);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(Lf * Rf);

  todo_impl;
}

Obj obj_div(Obj lhs, Obj rhs) {
  if (lhs->ti.is(TypeKind::Int))
    return ObjInt::make(Li / Ri);

  if (lhs->ti.is(TypeKind::Float))
    return ObjFloat::make(Lf / Rf);

  todo_impl;
}

Obj obj_mod(Obj lhs, Obj rhs) {
  debug(assert(lhs->ti.is(TypeKind::Int)));

  return ObjInt::make(Li % Ri);
}

Obj obj_lshift(Obj lhs, Obj rhs) {
  return ObjInt::make(Li << Ri);
}

Obj obj_rshift(Obj lhs, Obj rhs) {
  return ObjInt::make(Li >> Ri);
}

Obj obj_compare(CompareExprKind kind, Obj lhs, Obj rhs) {
  switch (kind) {
    case CompareExprKind::CMP_Bigger:
      if (lhs->as_int())
        return ObjBool::make(Li > Ri);
      else
        return ObjBool::make(Lf > Rf);

    case CompareExprKind::CMP_BiggerOrEqual:
      if (lhs->as_int())
        return ObjBool::make(Li >= Ri);
      else
        return ObjBool::make(Lf >= Rf);
  }

  return nullptr;
}

Obj obj_equal(Obj lhs, Obj rhs) {
  return ObjBool::make(lhs->equals(rhs));
}

Obj obj_bitand(Obj lhs, Obj rhs) {
  return ObjInt::make(Li & Ri);
}

Obj obj_bitor(Obj lhs, Obj rhs) {
  return ObjInt::make(Li | Ri);
}

Obj obj_bitxor(Obj lhs, Obj rhs) {
  return ObjInt::make(Li ^ Ri);
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

  auto nd_main_func = this->program->nd_program_main;

  (void)main_stack;
  // todo append [argc, argv] to main_stack

  main_stack.objects.resize(nd_main_func->nd_func_lvar_count);

  Obj result = nullptr;

  for (auto&& item : nd_main_func->nd_func_body->nd_elements) {
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

    case ND_Identifier:
    case ND_ScopeResol:
      alertmsg("converting is not implemented in Sema!");
      panic;

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

    case ND_Variable: {
      if (node->nd_variable_is_global)
        return this->global_variables[node->nd_variable_offset];

      return this->get_current_call_stack().get(node->nd_variable_offset);
    }

    case ND_Functor: {

      todo_impl;
    }

    case ND_EnumeratorName: {
      auto obj =
          ObjEnumerator::make(node->nd_enumerator_enum_node, node->nd_enumerator_index);

      return obj;
    }

    //
    // Call function
    //
    case ND_CallFunc: {
      Vec<Obj> args;

      for (auto&& item : node->nd_callfunc_args)
        args.emplace_back(this->eval_expr(item));

      return this->eval_call_func(node, args);
    }

    case ND_MemberAccess: {
      auto left = this->eval_expr(node->nd_lhs);

      if (left->ti.is(TypeKind::Instance))
        return left->as_instance()->members[node->nd_member_access_index];

      else if (left->ti.is(TypeKind::Enumerator))
        return left->as_enumerator()->data[node->nd_member_access_index];

      todo_impl;
    }

    case ND_Subscript: {
      todo_impl;
    }

    //
    // Contruct enumerator with intializers
    //
    case ND_ConstructEnumeratorValue: {
      auto obj = ObjEnumerator::make(node->nd_construct_enumerator_enum_def,
                                     node->nd_construct_enumerator_index);

      obj->ti = node->evaluated_type;
      obj->data.emplace_back(this->eval_expr(node->nd_construct_enumerator_arg));

      return obj;
    }

    case ND_ConstructEnumeratorStruct: {
      auto obj = ObjEnumerator::make(node->nd_construct_enumerator_enum_def,
                                     node->nd_construct_enumerator_index);

      obj->ti = node->evaluated_type;

      for (auto&& arg : node->nd_construct_enumerator_struct_args)
        obj->data.emplace_back(this->eval_expr(arg->nd_callctor_init_value));

      return obj;
    }

    case ND_CallConstructor: {

      auto obj = ObjInstance::make(node->nd_callctor_referenced_def, {});

      obj->ti = node->nd_callctor_ctor_side->evaluated_type;
      obj->ti.kind = TypeKind::Instance;

      for (auto&& val : node->nd_callctor_initializers)
        obj->members.emplace_back(this->eval_expr(val->nd_callctor_init_value));

      return obj;
    }

    case ND_ExprIf:
      if (this->eval_expr(node->nd_if_cond)->as_bool()->val)
        return this->eval_expr(node->nd_if_then);

      return this->eval_expr(node->nd_if_else);

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
  auto func = node->nd_callfunc_callee_userdef;

  if (!func) {
    // => no pointer to user-defined function, call builtin

    return node->nd_callfunc_callee_builtin->call(*this, node, args);
  }

  // if got pointer, call user-defined function
  auto& stack = this->push_stack();

  stack.objects.resize(func->nd_func_lvar_count);

  // for (auto&& arg : args)
  //   this->push(arg);

  for (size_t i = 0; i < args.size(); i++)
    stack.objects[i] = args[i];

  this->eval_block(func->nd_func_body);

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

    case ND_Switch:
      todo_impl;

    case ND_Match: {

      todo_impl;
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
  if (auto const x = node->nd_let_init)
    this->get_current_call_stack().objects[node->nd_let_offset] = this->eval_expr(x);
}

} // namespace fire