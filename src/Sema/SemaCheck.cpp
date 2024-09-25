#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

void Sema::check_full() {
  alert;

  alert;
  this->check(this->root);
}

void Sema::check(ASTPointer ast) {

  if (!ast) {
    return;
  }

  printkind;

  switch (ast->kind) {

  case ASTKind::Function: {
    alert;
    auto x = ASTCast<AST::Function>(ast);

    this->EnterScope(x);

    alert;
    auto func = this->get_func(x);

    alert;
    if (func->func->is_templated) {
      alert;
      break;
    }

    alert;
    assert(func != nullptr);

    func->result_type = this->evaltype(x->return_type);

    alert;
    AST::walk_ast(func->func->block,
                  [&func](AST::ASTWalkerLocation loc, ASTPointer _ast) {
                    if (loc == AST::AW_Begin && _ast->kind == ASTKind::Return) {
                      func->return_stmt_list.emplace_back(ASTCast<AST::Statement>(_ast));
                    }
                  });

    // alert;
    // for (auto&& arg : x->arguments) {
    //   alert;
    //   func->arg_types.emplace_back(this->evaltype(arg.type));
    // }

    alert;
    for (auto&& ret : func->return_stmt_list) {
      auto expr = ret->As<AST::Statement>()->get_expr();
      alert;

      if (auto type = this->evaltype(expr); !type.equals(func->result_type)) {
        alert;
        if (func->result_type.equals(TypeKind::None)) {
          Error(ret->token, "expected ';' after this token")();
        }
        else if (!expr) {
          Error(ret->token, "expected '" + func->result_type.to_string() +
                                "' type expression after this token")
              .emit();
        }
        else {
          Error(expr, "expected '" + func->result_type.to_string() +
                          "' type expression, but found '" + type.to_string() + "'")
              .emit();
        }

        goto _return_type_note;
      }
    }

    alert;
    if (!func->result_type.equals(TypeKind::None)) {
      if (func->return_stmt_list.empty()) {
        Error(func->func->token, "function must return value of type '" +
                                     func->result_type.to_string() +
                                     "', but don't return "
                                     "anything.")
            .emit();

      _return_type_note:
        Error(func->func->return_type, "specified here").emit(Error::ErrorLevel::Note);
      }
      else if (auto block = func->func->block;
               (*block->list.rbegin())->kind != ASTKind::Return) {
        Error(block->endtok, "expected return-statement before this token")();
      }
    }

    alert;
    this->check(x->block);

    this->LeaveScope(x);

    break;
  }

  case ASTKind::Block: {
    auto x = ASTCast<AST::Block>(ast);

    this->EnterScope(x);

    for (auto&& e : x->list)
      this->check(e);

    this->LeaveScope(x);

    break;
  }

  case ASTKind::Vardef: {

    auto x = ASTCast<AST::VarDef>(ast);

    this->check(x->type);
    this->check(x->init);

    break;
  }

  case ASTKind::Throw:
  case ASTKind::Return: {
    auto x = ASTCast<AST::Statement>(ast);

    this->check(x->get_expr());

    break;
  }

  default:
    this->evaltype(ast);
  }
}
} // namespace fire::semantics_checker