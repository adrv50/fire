#include <cassert>
#include "alert.h"
#include "Error.h"
#include "Builtins.h"
#include "Sema.h"

static bool _is_in_func_keep = false;

Sema::VarInfo::VarInfo()
    : name(""),
      ti(TypeKind::None) {
}

Sema::VarInfo::VarInfo(string const& name, TypeInfo const& ti)
    : name(name),
      ti(ti) {
}

//
// VarList::operator[]:
//   get variable by index.
//
Sema::VarInfo& Sema::VarList::operator[](size_t index) {
  return this->variables[index];
}

//
// VarList::append:
//   append variable to list.
//
Sema::VarInfo& Sema::VarList::append(VarInfo const& var) {
  auto size = this->variables.size();

  auto& emplaced = this->variables.emplace_back(var);

  emplaced.index = size;

  return emplaced;
}

//
// VarList::find:
//   find variable by name.
//
Sema::VarInfo* Sema::VarList::find(string const& name) {
  for (auto&& var : this->variables)
    if (var.name == name)
      return &var;

  return nullptr;
}

Sema::VarList::VarList()
    : variables() {
}

Sema::VarList::VarList(Vec<VarInfo> const& variables)
    : variables(variables) {
}

//
// Scope::get_name:
//   get name of scope.
//
string Sema::Scope::get_name() const {
  switch (this->type) {
  case SC_Function:
    return this->node->nd_func_name->str;

  case SC_Enum:
    return this->node->nd_enum_name->str;

  case SC_Class:
    return this->node->nd_class_name->str;

  case SC_Struct:
    return this->node->nd_struct_name->str;
  }

  return "";
}

//
// Scope::find_var:
//   find variable by name.
//
Sema::VarInfo* Sema::Scope::find_var(string const& name) {
  for (auto& var : this->variables)
    if (var.name == name)
      return &var;

  return nullptr;
}

//
// Scope::find_func:
//   find function by name.
//
Sema::Scope* Sema::Scope::find_func(string const& name) {
  for (auto& func : this->functions)
    if (func->get_name() == name)
      return func;

  return nullptr;
}

Sema::Scope* Sema::Scope::make_scope(Node* node) {
  switch (node->kind) {
  case ND_Program:
    break;

  case ND_Function: {
    auto scope = new Scope(SC_Function, node);

    scope->node = node;

    return scope;
  }

  case ND_Enum:
    todo_impl;
    break;

  case ND_Struct:
    todo_impl;
    break;

  case ND_Class:
    todo_impl;
    break;
  }

  auto scope = new Scope(SC_Global, node);

  for (auto&& item : node->nd_items) {
    if (item->is(ND_Function)) {
      auto fnscope = Scope::make_scope(item);

      scope->append(fnscope);
      scope->functions.emplace_back(fnscope);
    }
  }

  return scope;
}

Sema::Scope::Scope(ScopeType type, Node* node)
    : type(type),
      node(node) {
}

// ---------------------------------
//  SemaContext::SemaContext
// ---------------------------------
Sema::Scope* Sema::SemaContext::enter(Node* node) {
  this->cur_scope = this->cur_scope->find(node);

  assert(this->cur_scope);

  // this->cur_scope->letvp = this->cur_scope->variables.begin().base();

  if (node->is(ND_Function)) {
    _cur_func_keep = this->cur_func;

    this->cur_func = this->cur_scope;
  }

  return this->cur_scope;
}

// ---------------------------------
//  SemaContext::leave
// ---------------------------------
void Sema::SemaContext::leave() {
  if (this->cur_scope->node->is(ND_Function)) {
    this->cur_func = _cur_func_keep;
  }

  this->cur_scope = this->cur_scope->parent;
}

// ---------------------------------
//  ctor for Sema
// ---------------------------------
Sema::Sema(Node* root)
    : root(root),
      root_scope(Scope::make_scope(root)),
      ctx(root_scope) {
}

// ---------------------------------
//  enter_func
// ---------------------------------
Sema::Scope* Sema::enter_func(Node* func) {
  _is_in_func_keep = this->ctx.is_in_func;

  this->ctx.is_in_func = true;
  return this->ctx.enter(func);
}

// ---------------------------------
//  leave_func
// ---------------------------------
void Sema::leave_func(Node* func) {
  assert(this->ctx.cur_scope->node == func);

  this->ctx.leave();
  this->ctx.is_in_func = _is_in_func_keep;
}

//
// check_full:
//   check all nodes.
//
void Sema::check_full() {

  bool is_defined_main = false;

  for (auto&& item : this->root->nd_items) {
    switch (item->kind) {
    case ND_Let:
      this->check_let(item);
      break;

    case ND_Function:
      if (item->nd_func_name->str == "main") {
        is_defined_main = true;
        this->root->nd_program_main = item;
      }

      this->check_func(item);
      break;

    default:
      todo_impl;
      break;
    }
  }

  if (!is_defined_main) {
    Error(this->root->tok, "entry point 'main' is not defined").crash();
  }
}

//
// check_func:
//   check function.
//
void Sema::check_func(Node* func) {
  auto fnscope = this->enter_func(func);

  // check function name duplicate
  // todo

  // check function args
  for (auto&& arg : func->nd_func_args) {
    auto argtype = this->eval_type_ti(arg->nd_func_arg_type);

    auto& argvar =
        fnscope->variables.append(VarInfo(arg->nd_func_arg_name->str, argtype));

    argvar.decl = arg;
    argvar.is_type_deducted = true;

    fnscope->arg_types.emplace_back(argtype);
  }

  // check result type
  if (func->nd_func_result_type)
    fnscope->ti = this->eval_type_ti(func->nd_func_result_type);

  // check function body
  this->check_block(func->nd_func_body);

  if (!fnscope->ti.equals(TypeKind::None) && fnscope->ret_stmt_list.empty()) {
    Error(func, "function must return any value, but return nothing.")
        .add_note(func->nd_func_result_type, "specified here")
        .emit();
  }

  this->leave_func(func);
}

// ----------------------------------
//  check_stmt:
//    check statement nodes.
// ----------------------------------
void Sema::check_stmt(Node* stmt) {

  switch (stmt->kind) {

  case ND_Let:
    this->check_let(stmt);
    break;

  //
  // if-statement
  case ND_If:
    if (!this->eval_expr_ti(stmt->nd_if_cond).is(TypeKind::Bool))
      Error(stmt->tok, "if condition must be bool type").crash();

    this->check_block(stmt->nd_if_then);

    if (stmt->nd_if_else)
      this->check_block(stmt->nd_if_else);

    break;

  //
  // while-statement
  case ND_While:
    if (!this->eval_expr_ti(stmt->nd_while_cond).is(TypeKind::Bool))
      Error(stmt->tok, "while condition must be bool type").crash();

    this->check_block(stmt->nd_while_body);

    break;

  case ND_Block:
    this->check_block(stmt);
    break;

  //
  // return-statement
  case ND_Return: {
    auto cur_fn = this->ctx.cur_func;

    if (!cur_fn) {
      Error(stmt->tok, "cannot use 'return' outside of function").crash();
    }

    cur_fn->ret_stmt_list.emplace_back(stmt);

    auto const& expected = cur_fn->fnscope_ret_type;

    // take value
    if (stmt->nd_return_expr) {
      auto retval = this->eval_expr_ti(stmt->nd_return_expr);

      //
      // don't match to specified type
      if (!retval.equals(expected))
        Error(stmt->tok, "expected '" + expected.to_string() +
                             "' type expression, but found '" +
                             retval.to_string() + "'")
            .crash();
    }

    // don't take value
    else {
      // => is func side unspecified or None ?

      if (cur_fn->node->nd_func_result_type &&
          !expected.equals(TypeKind::None)) {
        Error(stmt->tok, "cannot take value in return statement. (function '" +
                             cur_fn->node->nd_func_name->str +
                             "' must return none)")
            .crash();
      }
    }

    break;
  }

  //
  // expression statement
  default:
    this->eval_expr_ti(stmt);
  }
}

//
// check_let:
//   check let statement.
//
void Sema::check_let(Node* let) {

  // todo: check let name duplicate

  TypeInfo ti;

  // if type is specified, evaluate type and check between initializer
  if (let->nd_let_type) {
    ti = this->eval_type_ti(let->nd_let_type);

    if (let->nd_let_init)
      if (!ti.equals(this->eval_expr_ti(let->nd_let_init)))
        Error(let->tok, "type mismatch").crash();
  }

  // if type is not specified, evaluate initializer type
  else if (let->nd_let_init) {
    ti = this->eval_expr_ti(let->nd_let_init);
  }

  // if not specified both, delay type deduction
  else {
    Error(let->tok, "delay type deduction is still not implemented").emit();

    return;
  }

  auto& var = this->get_cur_scope()->variables.append(
      VarInfo(let->nd_let_name->str, ti));

  var.is_type_deducted = true;
}

//
// check_block:
//   check block statement.
//
void Sema::check_block(Node* block) {
  for (auto&& stmt : block->nd_items)
    this->check_stmt(stmt);
}

//
// eval_expr_ti:
//   evaluate expression type info.
//
TypeInfo Sema::eval_expr_ti(Node* node) {

  switch (node->kind) {

  case ND_Value:
    return node->nd.obj->ti;

  case ND_Identifier:
  case ND_ScopeResol: {

    auto res = node->is(ND_Identifier)
                   ? this->find_name(node, this->get_cur_scope(), false, true)
                   : this->scope_resolution(node);

    if (res.var) {
      if (!res.var->is_type_deducted)
        Error(node, "cannot use variable before type deducted").crash();

      node->id_kind = NodeIdentifierKind::ID_Var;

      node->nd_variable_offset = res.var->index;
      node->nd_variable_is_global = (res.scope == this->root_scope);

      return res.var->ti;
    }

    else if (res.func) {
      todo_impl;
    }

    Error(node, "cannot find name '" + res.name + "'").crash();
  }

  case ND_Not:
    todo_impl;
    break;

  case ND_Ref:
    todo_impl;
    break;

  case ND_Subscript:
    todo_impl;
    break;

  case ND_MemberAccess:
    todo_impl;
    break;

  case ND_CallFunc:
    return this->check_function_call(node);

  default:
    break;
  }

  auto lhs = this->eval_expr_ti(node->nd_lhs);
  auto rhs = this->eval_expr_ti(node->nd_rhs);

  if (!lhs.equals(rhs))
    Error(node->tok, "type mismatch").crash();

  node->nd.tk = lhs.kind;

  switch (node->kind) {
  case ND_Add:
    if (lhs.is(TypeKind::String))
      break;
    // fallthrough

  case ND_Sub:
  case ND_Mul:
  case ND_Div:
    if (!lhs.is_numeric())
      Error(node->tok,
            "cannot use arithmetic operator for type '" + lhs.to_string() + "'")
          .crash();
    break;

  case ND_Mod:
  case ND_BitAnd:
  case ND_BitOr:
  case ND_BitXor:
  case ND_LShift:
  case ND_RShift:
    if (!lhs.is(TypeKind::Int))
      Error(node->tok, "cannot use operator '" + node->tok->str +
                           "' for type '" + lhs.to_string() + "'")
          .crash();
    break;

  case ND_Compare:
    if (!lhs.is_numeric())
      Error(node->tok,
            "comparing objects of type '" + lhs.to_string() + "' is not valid.")
          .crash();
    break;

  case ND_Equal:
    break;

  case ND_Or:
  case ND_And:
    if (!lhs.is(TypeKind::Bool))
      Error(node->tok, "only can use operator 'or' or 'and' for bool type")
          .crash();
    break;

  case ND_Assign:
    break;
  }

  return lhs;
}

//
// eval_type_ti:
//   evaluate type info.
//
TypeInfo Sema::eval_type_ti(Node* node) {
  assert(node->is(ND_TypeName));

  auto const& name = node->tok->str;

  TypeInfo ti;

  if ((ti.kind = TypeInfo::get_kind_of_name(name)) == TypeKind::Unknown) {
    // todo: find user-defined

    Error(node->tok, "unknown type name '" + name + "'").crash();
  }

  for (auto&& arg : node->nd_type_template_args)
    ti.template_args.push_back(this->eval_type_ti(arg));

  ti.is_mutable = node->nd_type_is_mut;
  ti.is_reference = node->nd_type_is_ref;

  return ti;
}

//
// check_function_call:
//   check function call.
//
TypeInfo Sema::check_function_call(Node* call) {

  Vec<TypeInfo> call_args;

  for (auto&& arg : call->nd_callfunc_args)
    call_args.push_back(this->eval_expr_ti(arg));

  // auto res = this->find_name_wrap(call->nd_callfunc_callee);
  auto res = this->find_name(call->nd_callfunc_callee, this->get_cur_scope(),
                             false, true);

  auto callee = res.func;

  if (!callee) {
    auto id = call->nd_callfunc_callee->get_last_id();

    auto const& name = id->tok->str;

    for (auto&& bf : Builtins::get_builtin_functions()) {
      if (bf.name == name) {
        this->compare_call_arguments(call, call_args, bf.arg_types,
                                     bf.is_variable_args, nullptr, &bf);

        call->nd_callfunc_callee_builtin = &bf;

        return bf.ret_type;
      }
    }

    Error(id->tok, "cannot find function '" + id->tok->str + "'").crash();
  }

  this->compare_call_arguments(call, call_args, res.scope->arg_types,
                               callee->nd_func_is_variable_args, callee,
                               nullptr);

  call->nd_callfunc_callee_userdef = res.func;

  return this->eval_type_ti(callee->nd_func_result_type);
}

//
// compare_call_arguments
//

void Sema::compare_call_arguments(Node* cf, Vec<TypeInfo> const& call,
                                  Vec<TypeInfo> const& func,
                                  bool is_variable_args, Node* fn,
                                  Builtins::BuiltinFunc const* bfn) {

  if (!is_variable_args && func.size() < call.size()) {
    Error(cf->nd_callfunc_callee->get_last_id(), "too many arguments")
        .add_note(fn,
                  fn ? "defined here"
                     : ("builtin function '" + bfn->name + "' can take up to " +
                        std::to_string(bfn->arg_types.size()) + " arguments."),
                  ErrorType::Note)
        .crash();
  }

  if (call.size() < func.size()) {
    Error(cf->nd_callfunc_callee->get_last_id(), "too few arguments")
        .add_note(fn,
                  fn ? "defined here"
                     : ("least " + std::to_string(bfn->arg_types.size()) +
                        " arguments needed by builtin function '" + bfn->name +
                        "'"),
                  ErrorType::Note)
        .crash();
  }

  for (size_t i = 0; i < func.size(); i++) {
    auto& callarg = call[i];
    auto& funcarg = func[i];

    if (!callarg.equals(funcarg)) {
      Error(cf->nd_callfunc_callee->get_last_id(),
            "expected '" + funcarg.to_string() +
                "' type expression, but found '" + callarg.to_string() + "'")
          .add_note(fn ? fn->nd_func_args[i]->nd_func_arg_type : nullptr,
                    fn ? "defined here" : "definition is: " + bfn->to_string())
          .crash();
    }
  }
}

//
// get_cur_scope:
//   get current scope.
//
Sema::Scope*& Sema::get_cur_scope() {
  return this->ctx.cur_scope;
}

//
// find_scope:
//   find scope by predicate.
//
Sema::Scope* Sema::find_scope_if(std::function<bool(Scope*)> const& pred,
                                 Scope* from_this, bool from_root,
                                 bool reverse) {

  Scope* scope = nullptr;

  if (from_root)
    scope = this->root_scope;
  else if (!(scope = from_this))
    scope = this->get_cur_scope();

  while (!pred(scope)) {
    if (reverse) {
      if (scope->parent)
        scope = scope->parent;
      else
        return nullptr;
    }
    else {
      for (auto&& child : scope->childs)
        if (auto found = this->find_scope_if(pred, child, false, reverse))
          return found;

      return nullptr;
    }
  }

  return scope;
}

//
// scope_resolution:
//   wrapper for scope resolution operator.
//
Sema::NameFindResult Sema::scope_resolution(Node* sr, Scope* scope) {
  (void)scope;

  auto res =
      this->find_name(sr->nd_scope_resol_first,
                      scope ? scope : this->get_cur_scope(), false, false);

  for (auto&& id : sr->nd_scope_resol_idlist) {
    switch (res.type) {
    case NameFindResult::NA_NotFound:
      return res;

    case NameFindResult::NA_Var:
    case NameFindResult::NA_Func:
      Error(id->tok,
            "cannot use scope resolution operator for variable or function")
          .crash();

    case NameFindResult::NA_Enum:
    case NameFindResult::NA_Class:
    case NameFindResult::NA_Struct:
    case NameFindResult::NA_Namespace:
      break;
    }

    res = this->find_name(id, res.scope, false, false);
  }

  return res;
}

//
// find_name:
//   find name in current scope.
//
Sema::NameFindResult Sema::find_name(Node* id, Scope* from_this, bool from_root,
                                     bool reverse) {
  Scope* scope = from_this;

  if (!scope)
    scope = from_root ? this->root_scope : this->get_cur_scope();

  NameFindResult result{id->nd_id_name->str};

  this->find_scope_if(
      [&](Scope* scope) -> bool {
        if ((result.var = scope->find_var(id->tok->str))) {
          result.type = NameFindResult::NA_Var;
          result.scope = scope;
          return true;
        }

        if (auto fn = scope->find_func(id->tok->str)) {
          result.type = NameFindResult::NA_Func;
          result.scope = fn;
          result.func = fn->node;
          return true;
        }

        return false;
      },
      nullptr, from_root, reverse);

  if (reverse) {
    if (scope->parent)
      return this->find_name(id, scope->parent, false, true);
  }
  else {
    for (auto&& child : scope->childs)
      if ((result = this->find_name(id, child, false, false)).is_found())
        break;
  }

  return result;
}
