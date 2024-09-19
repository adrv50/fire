#include "AST.h"
#include "Evaluator.h"
#include "Error.h"
#include "Builtin.h"
#include "alert.h"

namespace fire::eval {

using namespace AST;

void Evaluator::do_eval() {
  this->PushStack(); // global variable

  for (auto&& x : this->root->list) {
    evaluate(x);
  }

  this->PopStack();
}

ObjPointer Evaluator::eval_member_access(ASTPtr<AST::Expr> ast) {

  auto obj = this->evaluate(ast->lhs);
  auto& name = ast->rhs->token.str;

  //
  // ObjInstance
  if (obj->is_instance()) {
    auto inst = PtrCast<ObjInstance>(obj);

    if (inst->member.contains(name)) {
      return inst->member[name];
    }

    for (auto&& mf : inst->member_funcs) {
      if (mf->GetName() == name) {
        return mf;
      }
    }
  }

  //
  // ObjEnumerator
  else if (obj->type.kind == TypeKind::Enumerator) {
    //
    // -->  .data

    if (name == "data") {
      return obj->As<ObjEnumerator>()->data;
    }
  }

  //
  // ObjModule
  else if (obj->type.kind == TypeKind::Module) {
    alert;

    auto mod = PtrCast<ObjModule>(obj);

    if (mod->variables.contains(name)) {
      alertmsg(name);
      return mod->variables[name];
    }

    for (auto&& objtype : mod->types) {
      if (objtype->GetName() == name)
        return objtype;
    }

    for (auto&& objfunc : mod->functions) {
      alert;

      if (objfunc->GetName() == name)
        return objfunc;
    }
  }

  //
  // ObjType
  else if (obj->type.kind == TypeKind::TypeName) {

    auto typeobj = obj->As<ObjType>();

    if (typeobj->ast_class) {
      for (auto&& func : typeobj->ast_class->get_member_functions()) {
        if (func->GetName() == name)
          return ObjNew<ObjCallable>(func);
      }
    }

    else if (typeobj->ast_enum) {
      for (int index = 0; auto&& e : typeobj->ast_enum->enumerators->list) {
        if (e->As<AST::Variable>()->GetName() == name)
          return ObjNew<ObjEnumerator>(typeobj->ast_enum, index);

        index++;
      }
    }
  }

  ObjPtr<ObjCallable> callable;

  if (auto [fn_ast, blt] = this->find_func(name); fn_ast)
    callable = ObjNew<ObjCallable>(fn_ast);
  else if (blt)
    callable = ObjNew<ObjCallable>(blt);
  else {
    Error(ast->token, "not found the name '" + ast->rhs->token.str + "' in `" +
                          obj->type.to_string() + "` type object.")();
  }

  callable->selfobj = obj;
  callable->is_member_call = true;

  return callable;
}

ObjPointer& Evaluator::eval_as_writable(ASTPointer ast) {
  using Kind = ASTKind;

  switch (ast->kind) {
  case Kind::Variable: {
    auto x = ast->As<AST::Variable>();
    auto pvar = this->find_var(x->GetName());

    if (!pvar) {
      Error(ast->token, "undefined variable name")();
    }

    return *pvar;
  }

  case Kind::IndexRef: {
    auto x = ast->As<AST::Expr>();

    auto& obj = this->eval_as_writable(x->lhs);
    auto index = this->evaluate(x->rhs);

    (void)obj;
    (void)index;

    todo_impl;
  }

  case Kind::MemberAccess: {
    auto x = ast->As<AST::Expr>();
    auto& obj = this->eval_as_writable(x->lhs);
    auto name = x->rhs->token.str;

    if (obj->type.kind == TypeKind::Instance) {
      auto inst = obj->As<ObjInstance>();

      if (inst->member.contains(name))
        return inst->member[name];

      Error(x->rhs->token,
            "class '" + inst->ast->GetName() + "' don't have member '" + name + "'")();
    }

    else if (obj->type.kind == TypeKind::Enumerator && name == "data") {
      return obj->As<ObjEnumerator>()->data;
    }
  }
  }

  Error(ast, "cannot writable")();
}

ObjPointer Evaluator::evaluate(ASTPointer ast) {
  using Kind = ASTKind;

  if (!ast)
    return ObjNew<ObjNone>();

  switch (ast->kind) {
  case Kind::TypeName: {
    // clang-format off
    static std::map<TypeKind, char const*> kind_name_map {
      { TypeKind::None,       "none" },
      { TypeKind::Int,        "int" },
      { TypeKind::Float,      "float" },
      { TypeKind::Size,       "usize" },
      { TypeKind::Bool,       "bool" },
      { TypeKind::Char,       "char" },
      { TypeKind::String,     "string" },
      { TypeKind::Vector,     "vector" },
      { TypeKind::Tuple,      "tuple" },
      { TypeKind::Dict,       "dict" },
      { TypeKind::Instance,   "instance" },
      { TypeKind::Module,     "module" },
      { TypeKind::Function,   "function" },
      { TypeKind::Module,     "module" },
      { TypeKind::TypeName,   "type" },
    };
    // clang-format on

    auto x = ast->As<AST::TypeName>();
    std::string name = x->GetName();

    TypeInfo type;

    for (auto&& [key, val] : kind_name_map) {
      if (val == name) {
        alert;
        return ObjNew<ObjType>(key);
      }
    }

    auto obj = ObjNew<ObjType>(TypeKind::TypeName);
    obj->typeinfo.name = name;

    alertmsg(name);

    if (auto [C, E] = this->find_class_or_enum(name); C) {
      obj->ast_class = C;
    }
    else if (E) {
      obj->ast_enum = E;
    }
    else {
      Error(ast->token, "type name not found")();
    }

    return obj;
  }

  case Kind::Value:
    return ASTCast<Value>(ast)->value;

  case Kind::Variable: {
    auto x = ASTCast<Variable>(ast);
    auto name = x->GetName();

    auto pobj = this->find_var(name);

    // found variable
    if (pobj) {
      if (*pobj)
        return *pobj;

      Error(ast->token, "use variable before assignment")();
    }

    // find function
    if (auto [func, bfn] = this->find_func(name); func) {
      return ObjNew<ObjCallable>(func);
    }
    else if (bfn) {
      // builtin-func
      return ObjNew<ObjCallable>(bfn);
    }

    // find class or enum
    if (auto [C, E] = this->find_class_or_enum(name); C) {
      return ObjNew<ObjType>(C);
    }

    else if (E) {
      return ObjNew<ObjType>(E);
    }

    Error(ast->token, "no name defined")();
  }

  case Kind::Array: {
    auto x = ASTCast<Array>(ast);
    auto obj = ObjNew<ObjIterable>(TypeKind::Vector);

    for (auto&& e : x->elements)
      obj->Append(this->evaluate(e));

    return obj;
  }

  case Kind::CallFunc: {
    auto cf = ASTCast<CallFunc>(ast);

    auto obj = this->evaluate(cf->expr);

    ObjVector args;

    for (auto&& x : cf->args) {
      args.emplace_back(
          this->evaluate(x->kind == ASTKind::SpecifyArgumentName ? x->As<AST::Expr>()->rhs : x));
    }

    // type
    if (obj->type.kind == TypeKind::TypeName) {
      auto typeobj = obj->As<ObjType>();

      // class --> make instance
      if (typeobj->ast_class) {
        auto inst = this->new_class_instance(typeobj->ast_class);

        if (inst->have_constructor()) {
          args.insert(args.begin(), inst);

          this->call_function_ast(true, inst->get_constructor(), cf, args);
        }
        else if (args.size() >= 1) {
          Error(cf->args[0]->token, "class '" + typeobj->ast_class->GetName() +
                                        "' don't have constructor, but given arguments.")();
        }

        return inst;
      }
    }

    // not callable object --> error
    if (!obj->is_callable()) {
      Error(cf->expr->token, "`" + obj->type.to_string() + "` is not callable")();
    }

    auto cb = obj->As<ObjCallable>();

    if (cb->is_member_call) {
      args.insert(args.begin(), cb->selfobj);
    }

    // builtin func
    if (cb->builtin) {

      if (args.size() < cb->builtin->argcount) {
        Error(ast->token, "too few arguments")();
      }
      else if (!cb->builtin->is_variable_args && args.size() > cb->builtin->argcount) {
        Error(ast->token, "too many arguments")();
      }

      return cb->builtin->Call(cf, std::move(args));
    }

    return this->call_function_ast(cb->is_member_call, cb->func, cf, args);
  }

  case Kind::MemberAccess: {
    auto ret = this->eval_member_access(ASTCast<AST::Expr>(ast));

    if (!ret)
      Error(ast->As<AST::Expr>()->rhs->token, "use variable before assignment")();

    return ret;
  }

  case Kind::IndexRef: {
    auto x = ast->as_expr();

    auto content = this->evaluate(x->lhs);
    auto index_obj = this->evaluate(x->rhs);

    if (!index_obj->is_int())
      Error(ast->token, "indexer must be int.")();

    auto index = index_obj->As<ObjPrimitive>()->vi;

    if (content->is_vector() || content->is_string()) {
      return content->As<ObjIterable>()->list[index];
    }

    Error(ast->token, "'" + content->type.to_string() + "' type object is not subscriptable")();
  }

  case Kind::Assign: {
    auto x = ASTCast<AST::Expr>(ast);

    return this->eval_as_writable(x->lhs) = this->evaluate(x->rhs);
  }

  case Kind::Block: {
    auto _block = ASTCast<AST::Block>(ast);
    auto& stack = this->GetCurrentStack();

    for (auto&& x : _block->list) {
      this->evaluate(x);

      if (stack.is_returned)
        break;

      if (this->loop_stack.size())
        if (auto& L = this->GetCurrentLoop(); L.is_breaked || L.is_continued)
          break;
    }

    break;
  }

  case Kind::Vardef: {
    auto x = ASTCast<AST::VarDef>(ast);
    auto name = x->GetName();

    auto& stack = this->GetCurrentStack();

    auto pvar = stack.find_variable(name);

    if (!pvar)
      pvar = &stack.append(name);

    if (x->init)
      pvar->value = this->evaluate(x->init);

    break;
  }

  case Kind::Return: {
    auto& stack = this->GetCurrentStack();

    stack.result = this->evaluate(ast->as_stmt()->get_expr());

    stack.is_returned = true;

    break;
  }

  case Kind::Break:
  case Kind::Continue:
    if (this->loop_stack.empty())
      Error(ast->token, "cannot use in out of loop statement")();

    (ast->kind == Kind::Break ? this->GetCurrentLoop().is_breaked
                              : this->GetCurrentLoop().is_continued) = true;

    break;

  case Kind::Throw:
    throw this->evaluate(ast->as_stmt()->get_expr());

  case Kind::If: {
    auto x = ast->as_stmt();
    auto data = x->get_data<AST::Statement::If>();

    auto cond = this->evaluate(data.cond);

    if (cond->As<ObjPrimitive>()->vb)
      this->evaluate(data.if_true);
    else if (data.if_false)
      this->evaluate(data.if_false);

    break;
  }

  case Kind::For: {
    auto s = ast->as_stmt();
    auto data = s->get_data<AST::Statement::For>();

    for (auto&& x : data.init)
      if (x)
        this->evaluate(x);

    for (ObjPointer cond;;) {
      cond = this->evaluate(data.cond);

      if (!cond->is_boolean())
        Error(data.cond, "expected boolean expression")();

      if (!cond->As<ObjPrimitive>()->vb)
        break;

      this->evaluate(data.block);
    }

    break;
  }

  case Kind::TryCatch: {
    auto data = ASTCast<AST::Statement>(ast)->get_data<Statement::TryCatch>();

    try {
      this->evaluate(data.tryblock);
    }
    catch (ObjPointer object) {
      auto& stack = this->PushStack();
      stack.append(data.varname.str, object);

      this->evaluate(data.catched);

      this->PopStack();
    }

    break;
  }

  case Kind::Function:
  case Kind::Enum:
  case Kind::Class:
    break;

  default:
    if (ast->is_expr)
      return this->eval_expr(ASTCast<AST::Expr>(ast));
  }

  return ObjNew<ObjNone>();
}

Evaluator::Evaluator(ASTPtr<AST::Program> prg)
    : root(prg) {
  for (auto&& ast : prg->list) {
    switch (ast->kind) {
    case ASTKind::Function:
      alertmsg(ast->As<AST::Named>()->GetName());
      this->functions.emplace_back(ASTCast<AST::Function>(ast));
      break;

    case ASTKind::Class:
      alertmsg(ast->As<AST::Named>()->GetName());
      this->classes.emplace_back(ASTCast<AST::Class>(ast));
      break;

    case ASTKind::Enum:
      alertmsg(ast->As<AST::Named>()->GetName());
      this->enums.emplace_back(ASTCast<AST::Enum>(ast));
      break;
    }
  }
}

} // namespace fire::eval