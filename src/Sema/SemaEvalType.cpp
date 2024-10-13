#include "alert.h"
#include "Sema/Sema.h"
#include "Error.h"
#include "Builtin.h"

#include "ASTWalker.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

#define printkind alertmsg(static_cast<int>(ast->kind))
#define printkind_of(_A) alertmsg(static_cast<int>((_A)->kind))

namespace fire::semantics_checker {

TypeInfo Sema::eval_type(ASTPointer ast, SemaContext Ctx) {
  using Kind = ASTKind;

  if (!ast)
    return {};

  switch (ast->kind) {
  case Kind::Enum:
  case Kind::Function:
    return {};

  case Kind::Argument:
    ast = ast->As<AST::Argument>()->type;

  case Kind::TypeName:
    return this->eval_type_name(ASTCast<AST::TypeName>(ast));

  case Kind::Value: {
    return ast->as_value()->value->type;
  }

  case Kind::Variable:
  case Kind::FuncName:
  case Kind::BuiltinFuncName:
  case Kind::Identifier:
    return this->EvalID(AST::GetID(ast), Ctx).Type;

  case Kind::Array: {
    auto x = ast->As<AST::Array>();
    TypeInfo type = TypeKind::Vector;

    if (x->elements.empty()) {
      if (Ctx.TypeExpection) {
        return x->elem_type = *Ctx.TypeExpection->Expected;
      }

      throw Error(x->token, "cannot deduction element type")
          .AddNote("you can use 'array' keyword for specify type. (like "
                   "\"array<T>([]...])\")");
    }

    auto const& elemType = type.params.emplace_back(this->eval_type(x->elements[0]));

    x->elem_type = elemType;

    for (auto it = x->elements.begin() + 1; it != x->elements.end(); it++) {
      if (!elemType.equals(this->eval_type(*it))) {
        throw Error(*it, "expected '" + elemType.to_string() +
                             "' type expression as element in array");
      }
    }

    return type;
  }

  case Kind::OverloadResolutionGuide: {
    auto x = ASTCast<AST::Expr>(ast);

    todo_impl;
  }

  case Kind::Signature: {
    TypeInfo ret = TypeKind::Function;

    auto sig = ASTCast<AST::Signature>(ast);

    ret.params.emplace_back(this->eval_type(sig->result_type));

    for (auto&& t : sig->arg_type_list)
      ret.params.emplace_back(this->eval_type(t));

    return ret;
  }

  case Kind::ScopeResol: {
    auto scoperesol = ASTCast<AST::ScopeResol>(ast);

    todo_impl;
  }

  case Kind::Enumerator: {
    TypeInfo type = TypeKind::Enumerator;

    auto id = ast->GetID();

    type.type_ast = id->ast_enum;
    type.name = id->ast_enum->GetName();
    type.enum_index = id->index;

    return type;
  }

  case Kind::EnumName:
    return TypeInfo::from_enum(ast->GetID()->ast_enum);

  case Kind::ClassName:
    return TypeInfo::from_class(ast->GetID()->ast_class);

  case Kind::CallFunc: {
    auto call = ASTCast<AST::CallFunc>(ast);

    ASTPointer functor = call->callee;

    ASTPtr<AST::Identifier> id = nullptr; // -> functor (if id or scoperesol)

    TypeVec arg_types;

    for (ASTPointer arg : call->args) {
      arg_types.emplace_back(this->eval_type(arg));
    }

    SemaFunctionNameContext f_ctx;

    if (functor->is_ident_or_scoperesol() || functor->kind == ASTKind::MemberAccess) {
      id = AST::GetID(functor);

      f_ctx = {.CF = call, .ArgTypes = &arg_types, .MustDecideOneCandidate = false};

      Ctx.FuncNameCtx = &f_ctx;
    }

    TypeInfo functor_type = this->eval_type(functor, Ctx);

    if (functor->kind == ASTKind::MemberFunction) {
      call->IsMemberCall = true;

      call->args.insert(call->args.begin(), functor->as_expr()->lhs);

      functor->kind = ASTKind::FuncName;
    }

    switch (functor->kind) {
      //
      // ユーザー定義の関数名:
      //
    case ASTKind::FuncName: {

      assert(id->candidates.size() == 1);

      call->callee_ast = id->candidates[0];

      return functor_type.params[0];
    }

    case ASTKind::BuiltinMemberFunction:
    case ASTKind::BuiltinFuncName: {

      for (builtins::Function const* fn : id->candidates_builtin) {
        auto res = this->check_function_call_parameters(call->args, fn->is_variable_args,
                                                        fn->arg_types, arg_types, false);

        if (res.result == ArgumentCheckResult::Ok) {
          call->callee_builtin = fn;

          if (functor->kind == ASTKind::BuiltinMemberFunction)
            call->args.insert(call->args.begin(), functor->as_expr()->lhs);

          return fn->result_type;
        }
      }

      break;
    }

      //
      // class-name:
      //   --> create instance
      //
    case ASTKind::ClassName: {
      auto const& xmv = id->ast_class->member_variables;

      if (arg_types.size() < xmv.size()) {
        throw Error(functor, "too few constructor arguments");
      }
      else if (arg_types.size() > xmv.size()) {
        throw Error(functor, "too many constructor arguments");
      }

      for (i64 i = 0; i < (i64)xmv.size(); i++) {
        if (auto ttt = this->eval_type(xmv[i]->type ? xmv[i]->type : xmv[i]->init);
            !arg_types[i].equals(ttt)) {
          throw Error(call->args[i], "expected '" + ttt.to_string() +
                                         "' type expression, but found '" +
                                         arg_types[i].to_string() + "'");
        }
      }

      ast->kind = ASTKind::CallFunc_Ctor;

      if (auto E = Ctx.ExprCtx; E && E->IsClassInfoNeeded) {
        E->InstancableExprAST = ast;
        E->ExprInstanceClassPtr = id->ast_class;
      }

      return TypeInfo::make_instance_type(id->ast_class);
    }

    case ASTKind::Enumerator: {

      call->ast_enum = id->ast_enum;
      call->enum_index = id->index;

      auto& e = id->ast_enum->enumerators[id->index];

      if (arg_types.size() != e.types.size()) {
        throw Error(call, "too " +
                              string(arg_types.size() < e.types.size() ? "few" : "many") +
                              " arguments to construct enumerator '" +
                              AST::ToString(functor) + "'");
      }

      for (size_t i = 0; i < e.types.size(); i++) {
        if (auto et = this->eval_type(e.types[i]); !et.equals(arg_types[i])) {
          throw Error(call->args[i], "expected '" + et.to_string() +
                                         "' type expression, but found '" +
                                         arg_types[i].to_string() + "'");
        }
      }

      ast->kind = ASTKind::CallFunc_Enumerator;

      return functor_type;
    }

    default: {
      if (functor_type.kind != TypeKind::Function) {
        break;
      }

      if (functor_type.params.empty()) {
        panic;
      }

      TypeVec formal = functor_type.params;

      TypeInfo ret = formal[0];

      formal.erase(formal.begin());

      if (functor_type.is_member_func) {
        formal.erase(formal.begin());
      }

      auto res = this->check_function_call_parameters(
          call->args, functor_type.is_free_args, formal, arg_types, false);

      switch (res.result) {
      case ArgumentCheckResult::TooFewArguments:
        throw Error(call->token, "too few arguments");

      case ArgumentCheckResult::TooManyArguments:
        throw Error(call->token, "too many arguments");

      case ArgumentCheckResult::TypeMismatch:
        throw Error(call->args[res.index], "expected '" + formal[res.index].to_string() +
                                               "' type expression, but found '" +
                                               arg_types[res.index].to_string() + "'");
      }

      call->call_functor = true;

      return ret;
    }
    }

    throw Error(functor,
                "expected calllable expression or name of function or class or enum");
  }

  case Kind::CallFunc_Ctor: {
    return TypeInfo::from_class(ast->As<CallFunc>()->get_class_ptr());
  }

  case Kind::SpecifyArgumentName:
    return this->eval_type(ast->as_expr()->rhs);

  case Kind::IndexRef: {
    auto x = ASTCast<AST::Expr>(ast);

    auto arr = this->eval_type(x->lhs);

    switch (arr.kind) {
    case TypeKind::Vector:
      return arr.params[0];
    }

    throw Error(x->op, "'" + arr.to_string() + "' type is not subscriptable");
  }

  //
  // Member-Access expr
  ///
  case Kind::MemberAccess: {

    auto E = ASTCast<AST::Expr>(ast);

    SemaExprContext exprCtx;

    exprCtx = {
        .kind = SemaExprContext::EX_MemberRef,
        .E = E,
        .Left = E->lhs,
        .Right = E->rhs,
        .IsClassInfoNeeded = true,
    };

    Ctx.ExprCtx = &exprCtx;

    auto& LeftType = exprCtx.LeftType;

    LeftType = this->eval_type(E->lhs, Ctx);

    auto right = this->eval_type(E->rhs, Ctx);

    if (E->Is(ASTKind::RefMemberVar) && IsWritable(E->lhs)) {
      E->kind = ASTKind::RefMemberVar_Left;
    }

    return right;
  }

  case Kind::RefMemberVar: {
    return this->eval_type(
        ast->GetID()->ast_class->member_variables[ast->GetID()->index]->type);
  }

  case Kind::MemberFunction: {
    todo_impl;
  }

  case Kind::BuiltinMemberVariable:
    todo_impl;

  case Kind::BuiltinMemberFunction:
    todo_impl;

  case Kind::Not: {
    auto x = ASTCast<AST::Expr>(ast);

    auto type = this->eval_type(x->lhs);

    if (type.kind != TypeKind::Bool)
      throw Error(x->lhs, "expected boolean expression");

    return type;
  }

  case Kind::Assign: {
    auto x = ASTCast<AST::Expr>(ast);

    auto ctx = Ctx;

    SemaExprContext exprC = {
        .kind = SemaExprContext::EX_Assignment,
        .E = x,
        .Left = x->lhs,
        .Right = x->rhs,
        .LeftID = AST::GetID(x->lhs),
    };

    ctx.ExprCtx = &exprC;

    auto dest = this->eval_type(x->lhs, ctx);

    if (!this->IsWritable(x->lhs)) {
      throw Error(x->lhs, "expected writable expression");
    }

    if (exprC.AssignRightTypeToLVar) {
      debug(assert(exprC.TargetLVarPtr));

      exprC.TargetLVarPtr->deducted_type = this->eval_type(x->rhs);
      exprC.TargetLVarPtr->is_type_deducted = true;
    }
    else {
      this->ExpectType(dest, x->rhs);
    }

    return dest;
  }

  default:
    debug(if (!ast->IsExpr()) {
      Error(ast, "").emit();
      alertmsg(static_cast<int>(ast->kind));
      todo_impl;
    });

    return this->EvalExpr(ASTCast<AST::Expr>(ast));
  }

  return TypeKind::None;
}

} // namespace fire::semantics_checker