#include <cassert>
#include "alert.h"
#include "Token.h"
#include "Node.h"
#include "Error.h"
#include "Builtins.h"
#include "Sema.h"
#include "Object.h"

namespace sema {

// ---------------------------------
//  ctor for Sema
// ---------------------------------
Sema::Sema(Node* root)
    : root(root),
      root_scope(),
      ctx() {

  if (root->is(ND_Program)) {
    this->root_scope = Scope::make_scope(root);

    this->ctx.cur_scope = this->root_scope;
  }
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
          if (item->nd_func_is_template)
            Error(item->tok, "entry point 'main' cannot be templated").crash();

          is_defined_main = true;
          this->root->nd_program_main = item;
        }

        if (!item->nd_func_is_template)
          this->check_func(item);

        break;

      case ND_Enum:
        this->check_enum(item);
        break;

      case ND_Struct:
        this->check_struct(item);
        break;

      case ND_Class:
        this->check_class(item);
        break;

      case ND_Namespace:
        todo_impl;

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
// check_enum:
//   check enum.
//
void Sema::check_enum(Node* nd_enum) {
  for (auto&& en : nd_enum->nd_enum_enumerators) {
    if (en->nd_enumerator_is_value) {
      this->eval_type_ti(en->nd_enumerator_val_type);
    }
    else if (en->nd_enumerator_is_struct) {
      for (auto&& member : en->nd_enumerator_struct_members) {
        this->eval_type_ti(member->nd_struct_member_type);
      }
    }
  }
}

//
// check_struct:
//   check struct.
//
void Sema::check_struct(Node* nd_struct) {
  (void)nd_struct;

  todo_impl;
}

//
// check_class:
//   check class.
//
void Sema::check_class(Node* nd_class) {
  (void)nd_class;

  todo_impl;
}

//
// check_func:
//   check function.
//
void Sema::check_func(Node* func) {
  // if (func->nd_func_is_template)
  //   return;

  auto fnscope = this->enter_func(func);

  auto& fr = fnscope->fn_eval_record_ptr;

  if (!fr)
    fr = this->find_func_eval_record(func);

  if (!fr) {
    alert;

    fr = new SemaFunctionEvaluatedRecord();

    fr->node = func;
  }

  if (fnscope->arg_types.size() == func->nd_func_args.size()) {
    alert;

    fnscope->arg_types.clear();

    for (size_t i = 0; i < func->nd_func_args.size(); i++)
      fnscope->variables.variables.erase(fnscope->variables.begin());
  }

  // check function args
  for (auto&& arg : func->nd_func_args) {
    auto argtype = this->eval_type_ti(arg->nd_func_arg_type);

    fr->arg_types.emplace_back(argtype);

    auto& argvar =
        fnscope->variables.append(VarInfo(arg->nd_func_arg_name->str, argtype));

    argvar.decl = arg;
    argvar.is_type_deducted = true;

    fnscope->arg_types.emplace_back(argtype);
  }

  func->nd_func_args_ti = &fnscope->arg_types;

  // check result type
  if (func->nd_func_result_type) {
    fr->result_type = fnscope->ti = this->eval_type_ti(func->nd_func_result_type);

    func->nd_func_result_ti = &fnscope->ti;
  }
  else {
    func->nd_func_result_ti = &TypeInfo::static_none_type;
  }

  // check duplicate of name
  if (Vec<Scope*> chk_duplicate;
      this->find_function(chk_duplicate, fnscope->parent, func->nd_func_name->str,
                          fnscope->arg_types, fnscope->ti, fnscope) != 0) {
    Error(func, "redefinition of function name '" + func->nd_func_name->str +
                    "' with same signature")
        .add_note(chk_duplicate[0]->node->tok, "defined here")
        .crash();
  }

  // check function body
  this->check_block(func->nd_func_body);

  if (!fnscope->ti.equals(TypeKind::None) && fnscope->ret_stmt_list.empty()) {
    Error(func, "function must return any value, but return nothing.")
        .add_note(func->nd_func_result_type, "specified here")
        .emit();
  }

  this->leave_func(func);

  fnscope->checked = true;
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
                               "' type expression, but found '" + retval.to_string() +
                               "'")
              .crash();
      }

      // don't take value
      else {
        // => is func side unspecified or None ?

        if (cur_fn->node->nd_func_result_type && !expected.equals(TypeKind::None)) {
          Error(stmt->tok, "cannot take value in return statement. (function '" +
                               cur_fn->node->nd_func_name->str + "' must return none)")
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

  alert;

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

  auto& var = this->get_cur_scope()->variables.append(VarInfo(let->nd_let_name->str, ti));

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

} // namespace sema
