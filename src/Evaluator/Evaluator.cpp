#include "Builtin.h"
#include "Evaluator.h"
#include "Error.h"

#define CAST(T) auto x = ASTCast<AST::T>(ast)

namespace fire::eval {

ObjPtr<ObjNone> Evaluator::_None;

Evaluator::Evaluator() {
  _None = ObjNew<ObjNone>();
}

Evaluator::~Evaluator() {
}

ObjPtr<ObjInstance> Evaluator::CreateClassInstance(ASTPtr<AST::Class> ast) {

  auto obj = ObjNew<ObjInstance>(ast);

  if (ast->InheritBaseClassPtr) {
    obj->base_class_inst = CreateClassInstance(ast->InheritBaseClassPtr);
  }

  obj->member_variables.resize(ast->member_variables.size());

  for (size_t i = 0; i < ast->member_variables.size(); i++) {
    auto& mv = ast->member_variables[i];

    if (mv->init) {
      obj->member_variables[i] = this->evaluate(mv->init);
    }
  }

  return obj;
}

ObjPointer Evaluator::CreateDefaultValue(TypeInfo const& type) {

  switch (type.kind) {
  case TypeKind::None:
    return _None;

  case TypeKind::Int:
    return ObjNew<ObjPrimitive>((i64)0);

  case TypeKind::Float:
    return ObjNew<ObjPrimitive>((float)0);

  case TypeKind::Bool:
    return ObjNew<ObjPrimitive>((bool)0);

  case TypeKind::Char:
    return ObjNew<ObjPrimitive>((char16_t)0);

  case TypeKind::String:
    return ObjNew<ObjString>();

  case TypeKind::Vector:
    return ObjNew<ObjIterable>(TypeKind::Vector);

  case TypeKind::Tuple:
    todo_impl;

  case TypeKind::Dict:
    todo_impl;
  }

  return nullptr;
}

Evaluator::VarStackPtr Evaluator::push_stack(size_t var_count) {
  return this->var_stack.emplace_front(std::make_shared<VarStack>(var_count));
}

void Evaluator::pop_stack() {
  debug(assert(this->var_stack.size() >= 1));

  this->var_stack.pop_front();
}

VarStack& Evaluator::get_cur_stack() {
  return **this->var_stack.begin();
}

VarStack& Evaluator::get_stack(int distance) {
  auto it = this->var_stack.begin();

  for (int i = 0; i < distance; i++)
    it++;

  return **it;
}

ObjPointer& Evaluator::eval_as_left(ASTPointer ast) {

  switch (ast->kind) {
  case ASTKind::IndexRef: {
    auto ex = ast->as_expr();

    return this->eval_index_ref(this->eval_as_left(ex->lhs), this->evaluate(ex->rhs));
  }

  case ASTKind::RefMemberVar_Left: {

    auto id = ASTCast<AST::Identifier>(ast->as_expr()->rhs);

    return this->eval_member_ref(
        PtrCast<ObjInstance>(this->eval_as_left(ast->as_expr()->lhs)), id->ast_class,
        id->index);
  }
  }

  debug(assert(ast->kind == ASTKind::Variable));

  auto x = ast->GetID();

  return this->get_stack(x->distance).var_list[x->index + x->index_add];
}

ObjPointer& Evaluator::eval_index_ref(ObjPointer array, ObjPointer _index_obj) {
  assert(_index_obj->type.kind == TypeKind::Int);

  i64 index = _index_obj->As<ObjPrimitive>()->vi;

  switch (array->type.kind) {
  case TypeKind::Dict: {
    todo_impl;
  }
  }

  debug(assert(array->type.kind == TypeKind::Vector));

  return array->As<ObjIterable>()->list[(size_t)index];
}

ObjPointer& Evaluator::eval_member_ref(ObjPtr<ObjInstance> inst,
                                       ASTPtr<AST::Class> expected_class, int index) {

  while (inst->ast != expected_class) {
    inst = inst->base_class_inst;
  }

  return inst->get_mvar(index);
}

ObjPointer Evaluator::evaluate(ASTPointer ast) {
  using Kind = ASTKind;

  if (!ast) {
    return _None;
  }

  switch (ast->kind) {

  case Kind::Function:
  case Kind::Class:
  case Kind::Enum:
    break;

  case Kind::Identifier:
  case Kind::ScopeResol:
  case Kind::MemberAccess:
    // この２つの Kind は、意味解析で変更されているはず。
    // ここまで来ている場合、バグです。
    debug(Error(ast, "??").emit());
    panic;

  case Kind::Value: {
    return ast->as_value()->value;
  }

  case Kind::Variable:
  case Kind::RefMemberVar_Left:
    return this->eval_as_left(ast);

  case Kind::RefMemberVar: {
    auto id = ASTCast<AST::Identifier>(ast->as_expr()->rhs);

    return this->eval_member_ref(
        PtrCast<ObjInstance>(this->evaluate(ast->as_expr()->lhs)), id->ast_class,
        id->index);
  }

  case Kind::Array: {
    CAST(Array);

    auto obj = ObjNew<ObjIterable>(TypeInfo(TypeKind::Vector, {x->elem_type}));

    for (auto&& e : x->elements)
      obj->Append(this->evaluate(e));

    return obj;
  }

  case Kind::IndexRef: {
    auto ex = ast->as_expr();

    return this->eval_index_ref(this->evaluate(ex->lhs), this->evaluate(ex->rhs));
  }

  case Kind::LambdaFunc: {
    auto func = ASTCast<AST::Function>(ast);

    auto obj = ObjNew<ObjCallable>(func);

    obj->type.params = {this->evaluate(func->return_type)->type};

    for (auto&& arg : func->arguments)
      obj->type.params.emplace_back(this->evaluate(arg->type)->type);

    return obj;
  }

  case Kind::OverloadResolutionGuide:
    ast = ast->as_expr()->lhs;

  case Kind::FuncName: {
    auto id = ast->GetID();
    auto obj = ObjNew<ObjCallable>(id->candidates[0]);

    obj->type.params = id->ft_args;
    obj->type.params.insert(obj->type.params.begin(), id->ft_ret);

    return obj;
  }

  case Kind::BuiltinFuncName: {
    auto id = ast->GetID();
    auto obj = ObjNew<ObjCallable>(id->candidates_builtin[0]);

    obj->type.params = id->ft_args;
    obj->type.params.insert(obj->type.params.begin(), id->ft_ret);

    return obj;
  }

  case Kind::Enumerator: {
    return ObjNew<ObjEnumerator>(ast->GetID()->ast_enum, ast->GetID()->index);
  }

  case Kind::EnumName: {
    return ObjNew<ObjType>(ast->GetID()->ast_enum);
  }

  case Kind::ClassName: {
    return ObjNew<ObjType>(ast->GetID()->ast_class);
  }

  case Kind::MemberFunction: {
    return ObjNew<ObjCallable>(ast->GetID()->candidates[0]);
  }

  case Kind::BuiltinMemberVariable: {
    auto self = ast->as_expr()->lhs;
    auto id = ast->GetID();

    return id->blt_member_var->impl(self, this->evaluate(self));
  }

  case Kind::BuiltinMemberFunction: {
    auto expr = ast->as_expr();
    auto id = expr->GetID();

    auto callable = ObjNew<ObjCallable>(id->candidates_builtin[0]);

    callable->selfobj = this->evaluate(expr->lhs);
    callable->is_member_call = true;

    return callable;
  }

  case Kind::CallFunc: {
    CAST(CallFunc);

    ObjVector args;

    for (auto&& arg : x->args) {
      args.emplace_back(this->evaluate(arg));
    }

    auto _func = x->callee_ast;
    auto _builtin = x->callee_builtin;

    if (x->call_functor) {
      auto functor = PtrCast<ObjCallable>(this->evaluate(x->callee));

      if (functor->func)
        _func = functor->func;
      else
        _builtin = functor->builtin;
    }

    if (_builtin) {
      return _builtin->Call(x, std::move(args));
    }

    if (x->IsMemberCall) {
      auto _class = args[0]->As<ObjInstance>()->ast;

      string name = AST::GetID(x->callee)->GetName();

      for (auto&& mf : _class->member_functions) {
        if (mf->GetName() == name && mf != _func) {
          _func = mf;
          break;
        }
      }
    }

    auto stack = this->push_stack(x->args.size());

    if (this->var_stack.size() >= 1588) {
      throw Error(ast->token, "stack overflow");
    }

    this->call_stack.push_front(stack);

    stack->var_list = std::move(args);

    this->evaluate(_func->block);

    auto result = stack->func_result;

    this->pop_stack();
    this->call_stack.pop_front();

    return result ? result : _None;
  }

  case Kind::CallFunc_Ctor: {
    CAST(CallFunc);

    auto ast_class = x->get_class_ptr();

    auto inst = this->CreateClassInstance(ast_class);

    for (size_t i = 0; i < x->args.size(); i++) {
      inst->member_variables[i] = this->evaluate(x->args[i]);
    }

    return inst;
  }

  case Kind::CallFunc_Enumerator: {
    auto x = ASTCast<AST::CallFunc>(ast);

    auto obj = ObjNew<ObjEnumerator>(x->ast_enum, x->enum_index);

    if (x->ast_enum->enumerators[x->enum_index].data_type ==
        AST::Enum::Enumerator::DataType::Value) {
      obj->data = this->evaluate(x->args[0]);
    }
    else {
      auto list = ObjNew<ObjIterable>(TypeKind::Vector);

      for (auto&& arg : x->args)
        list->Append(this->evaluate(arg));

      obj->data = list;
    }

    return obj;
  }

  case Kind::Assign: {
    auto x = ast->as_expr();

    return this->eval_as_left(x->lhs) = this->evaluate(x->rhs);
  }

  case Kind::Return:
  case Kind::Throw:
  case Kind::Break:
  case Kind::Continue:
  case Kind::Block:
  case Kind::Namespace:
  case Kind::If:
  case Kind::Match:
  case Kind::While:
  case Kind::TryCatch:
  case Kind::Vardef:
    this->eval_stmt(ast);
    break;

  default:
    if (ast->IsExpr())
      return this->eval_expr(ASTCast<AST::Expr>(ast));

    alertexpr(static_cast<int>(ast->kind));
    todo_impl;
  }

  return _None;
}

} // namespace fire::eval