#include "AST.h"
#include "alert.h"

namespace fire::AST {

ASTPtr<Identifier> GetID(ASTPointer ast) {

  if( ast->IsConstructedAs(ASTKind::MemberAccess) )
    return GetID(ast->as_expr()->rhs);

  if( ast->IsConstructedAs(ASTKind::ScopeResol) )
    return ast->As<AST::ScopeResol>()->GetLastID();

  assert( ast->IsConstructedAs(ASTKind::Identifier) );

  return ASTCast<AST::Identifier>(ast);
}

bool Base::IsQualifiedIdentifier()  {
  return this->IsIdentifier() && this->GetID()->id_params.size() >= 1;
}

bool Base::IsUnqualifiedIdentifier() {
  return this->IsIdentifier() && this->GetID()->id_params.empty();
}


Expr const* Base::as_expr() const {
  return static_cast<Expr const*>(this);
}

Statement const* Base::as_stmt() const {
  return static_cast<Statement const*>(this);
}

Expr* Base::as_expr() {
  return static_cast<Expr*>(this);
}

Statement* Base::as_stmt() {
  return static_cast<Statement*>(this);
}

Value const* Base::as_value() const {
  return (Value const*)this;
}

Identifier* Base::GetID() {
  return nullptr;
}

i64 Base::GetChilds(ASTVector& out) const {

  (void)out;
  todo_impl;

  return 0;
}


// ----------------------------------- //

ASTPtr<Array> Array::New(Token tok) {
  return ASTNew<Array>(tok);
}

ASTPointer Array::Clone() const {
  auto x = New(this->token);

  for (ASTPointer const& e : this->elements)
    x->elements.emplace_back(e->Clone());

  return x;
}

Array::Array(Token tok)
    : Base(ASTKind::Array, tok) {
}

ASTPtr<CallFunc> CallFunc::New(ASTPointer expr, ASTVector args) {
  return ASTNew<CallFunc>(expr, std::move(args));
}

ASTPointer CallFunc::Clone() const {
  auto x = New(this->callee);

  for (ASTPointer const& a : this->args)
    x->args.emplace_back(a->Clone());

  x->callee_ast = this->callee_ast;
  x->callee_builtin = this->callee_builtin;

  return x;
}

ASTPtr<Class> CallFunc::get_class_ptr() const {
  return this->callee->GetID()->ast_class;
}

CallFunc::CallFunc(ASTPointer callee, ASTVector args)
    : Base(ASTKind::CallFunc, callee->token, callee->token),
      callee(callee),
      args(std::move(args)) {
}

ASTPtr<Expr> Expr::New(ASTKind kind, Token op, ASTPointer lhs, ASTPointer rhs) {
  return ASTNew<Expr>(kind, op, lhs, rhs);
}

ASTPointer Expr::Clone() const {
  return New(this->kind, this->op, this->lhs->Clone(), this->rhs->Clone());
}

Identifier* Expr::GetID() {
  return this->rhs->GetID();
}

Expr::Expr(ASTKind kind, Token optok, ASTPointer lhs, ASTPointer rhs)
    : Base(kind, optok),
      op(this->token),
      lhs(lhs),
      rhs(rhs) {
  this->_attr.IsExpr = true;
}

ASTPtr<Block> Block::New(Token tok, ASTVector list) {
  return ASTNew<Block>(tok, std::move(list));
}

ASTPointer Block::Clone() const {
  auto x = New(this->token);

  for (ASTPointer const& a : this->list)
    x->list.emplace_back(a->Clone());

  x->ScopeCtxPtr = this->ScopeCtxPtr;

  return x;
}

Block::Block(Token tok, ASTVector list)
    : Base(ASTKind::Block, tok),
      list(std::move(list)) {
}

ASTPtr<TypeName> TypeName::New(Token nametok) {
  return ASTNew<TypeName>(nametok);
}

TypeName::TypeName(Token name)
    : Named(ASTKind::TypeName, name),
      is_const(false) {
}

ASTPtr<VarDef> VarDef::New(Token tok, Token name, ASTPtr<TypeName> type,
                           ASTPointer init) {
  return ASTNew<VarDef>(tok, name, type, init);
}

VarDef::VarDef(Token tok, Token name, ASTPtr<TypeName> type, ASTPointer init)
    : Named(ASTKind::Vardef, tok, name),
      type(type),
      init(init) {
}

ASTPtr<Statement> Statement::NewIf(Token tok, ASTPointer cond, ASTPointer if_true,
                                   ASTPointer if_false) {
  return ASTNew<Statement>(ASTKind::If, tok, new If{cond, if_true, if_false});
}

ASTPtr<Statement> Statement::NewSwitch(Token tok, ASTPointer cond,
                                       Vec<Switch::Case> cases) {
  return ASTNew<Statement>(ASTKind::Switch, tok, new Switch{cond, std::move(cases)});
}

ASTPtr<Statement> Statement::NewWhile(Token tok, ASTPointer cond, ASTPtr<Block> block) {

  return ASTNew<Statement>(ASTKind::While, tok, new While{cond, block});
}

ASTPtr<Statement> Statement::NewTryCatch(Token tok, ASTPtr<Block> tryblock,
                                         vector<TryCatch::Catcher> catchers) {
  return ASTNew<Statement>(ASTKind::TryCatch, tok,
                           new TryCatch{tryblock, std::move(catchers)});
}

ASTPtr<Statement> Statement::New(ASTKind kind, Token tok, void* data) {
  return ASTNew<Statement>(kind, tok, data);
}

ASTPtr<Statement> Statement::NewExpr(ASTKind kind, Token tok, ASTPointer expr) {
  return ASTNew<Statement>(kind, tok, expr);
}

Statement::Statement(ASTKind kind, Token tok, void* data)
    : Base(kind, tok),
      _data(data) {
}

Statement::Statement(ASTKind kind, Token tok, ASTPointer expr)
    : Base(kind, tok),
      expr(expr) {
}

Statement::~Statement() {
  switch (this->kind) {
  case ASTKind::If:
    delete this->data_if;
    break;

  case ASTKind::Switch:
    delete this->data_switch;
    break;

  case ASTKind::While:
    delete this->data_while;
    break;

  case ASTKind::TryCatch:
    delete this->data_try_catch;
    break;
  }
}

ASTPointer Match::Clone() const {
  auto ast = New(this->token, this->cond->Clone(), {});

  for (auto&& p : this->patterns)
    ast->patterns.emplace_back(p.type, p.expr->Clone(),
                               ASTCast<AST::Block>(p.block->Clone()), p.everything,
                               p.is_eval_expr);

  return ast;
}

ASTPtr<Match> Match::New(Token tok, ASTPointer cond, Vec<Pattern> patterns) {
  return ASTNew<Match>(tok, cond, std::move(patterns));
}

Match::Match(Token tok, ASTPointer cond, Vec<Pattern> patterns)
    : Base(ASTKind::Match, tok),
      cond(cond),
      patterns(std::move(patterns)) {
}

ASTPtr<Argument> Argument::New(Token nametok, ASTPtr<TypeName> type) {
  return ASTNew<Argument>(nametok, type);
}

ASTPointer Argument::Clone() const {
  return New(this->name,
             ASTCast<AST::TypeName>(this->type ? this->type->Clone() : nullptr));
}

Argument::Argument(Token nametok, ASTPtr<TypeName> type)
    : Named(ASTKind::Argument, nametok),
      type(type) {
}


sema::FunctionScope* Function::GetScope() {
  return (sema::FunctionScope*)(this->ScopeCtxPtr);
}


ASTPtr<Function> Function::New(Token tok, Token name) {
  return ASTNew<Function>(tok, name);
}

ASTPtr<Function> Function::New(Token tok, Token name, ASTVec<Argument> args,
                               bool is_var_arg, ASTPtr<TypeName> rettype,
                               ASTPtr<Block> block) {
  return ASTNew<Function>(tok, name, std::move(args), is_var_arg, rettype, block);
}

ASTPtr<Argument>& Function::add_arg(Token const& tok, ASTPtr<TypeName> type) {
  return this->arguments.emplace_back(Argument::New(tok, type));
}

ASTPtr<Argument> Function::find_arg(std::string const& name) {
  for (auto&& arg : this->arguments)
    if (arg->GetName() == name)
      return arg;

  return nullptr;
}

ASTPointer Function::Clone() const {
  auto x = New(this->token, this->name);

  x->_Copy(*this);

  for (auto&& arg : this->arguments) {
    x->arguments.emplace_back(ASTCast<Argument>(arg->Clone()));
  }

  if (this->return_type)
    x->return_type = ASTCast<TypeName>(this->return_type->Clone());

  x->block = ASTCast<Block>(this->block->Clone());
  x->is_var_arg = this->is_var_arg;

  return x;
}

Function::Function(Token tok, Token name)
    : Function(tok, name, {}, false, nullptr, nullptr) {
}

Function::Function(Token tok, Token name, ASTVec<Argument> args, bool is_var_arg,
                   ASTPtr<TypeName> rettype, ASTPtr<Block> block)
    : Templatable(ASTKind::Function, tok, name),
      arguments(args),
      return_type(rettype),
      block(block),
      is_var_arg(is_var_arg) {
}

ASTPtr<Enum> Enum::New(Token tok, Token name) {
  return ASTNew<Enum>(tok, name);
}

ASTPointer Enum::Clone() const {
  auto x = New(this->token, this->name);

  x->_Copy(*this);

  for (auto&& e : this->enumerators)
    x->append(e);

  return x;
}

Enum::Enum(Token tok, Token name)
    : Templatable(ASTKind::Enum, tok, name) {
}

ASTPtr<Class> Class::New(Token tok, Token name) {
  return ASTNew<Class>(tok, name);
}

ASTPtr<Class> Class::New(Token tok, Token name, ASTVec<VarDef> member_variables,
                         ASTVec<Function> member_functions) {
  return ASTNew<Class>(tok, name, std::move(member_variables),
                       std::move(member_functions));
}

ASTPtr<VarDef>& Class::append_var(ASTPtr<VarDef> ast) {
  return this->member_variables.emplace_back(ast);
}

ASTPtr<Function>& Class::append_func(ASTPtr<Function> ast) {
  return this->member_functions.emplace_back(ast);
}

ASTPointer Class::Clone() const {
  auto x = New(this->token, this->name, CloneASTVec<VarDef>(this->member_variables),
               CloneASTVec<Function>(this->member_functions));

  x->_Copy(*this);

  return x;
}

Class::Class(Token tok, Token name, ASTVec<VarDef> member_variables,
             ASTVec<Function> member_functions)
    : Templatable(ASTKind::Class, tok, name),
      member_variables(std::move(member_variables)),
      member_functions(std::move(member_functions)) {
}

ASTPointer TypeName::Clone() const {
  auto x = New(this->name);

  for (auto&& p : this->type_params)
    x->type_params.emplace_back(ASTCast<TypeName>(p->Clone()));

  x->is_const = this->is_const;

  x->type = this->type;

  if (this->ast_class)
    x->ast_class = ASTCast<AST::Class>(this->ast_class->Clone());

  return x;
}

ASTPointer VarDef::Clone() const {
  return New(this->token, this->name,
             ASTCast<TypeName>(this->type ? this->type->Clone() : nullptr),
             this->init ? this->init->Clone() : nullptr);
}

ASTPointer Statement::Clone() const {
  switch (this->kind) {
  case ASTKind::If:
    return NewIf(this->token, this->data_if->cond, this->data_if->if_true,
                 this->data_if->if_false);

  case ASTKind::Switch: {
    auto d = this->data_switch;

    Vec<Switch::Case> _cases;

    for (auto&& c : d->cases)
      _cases.emplace_back(
          Switch::Case{.expr = c.expr->Clone(), .block = c.block->Clone()});

    return NewSwitch(this->token, d->cond->Clone(), std::move(_cases));
  }

  case ASTKind::While: {
    auto d = this->data_while;

    return NewWhile(this->token, d->cond->Clone(),
                    ASTCast<AST::Block>(d->block->Clone()));
  }

  case ASTKind::Break:
  case ASTKind::Continue:
    return New(this->kind, this->token, nullptr);

  case ASTKind::Throw:
  case ASTKind::Return:
    break;

  default:
    todo_impl;
  }

  return NewExpr(this->kind, this->token, this->expr->Clone());
}

ASTPtr<Signature> Signature::New(Token tok, ASTVec<TypeName> arg_type_list,
                                 ASTPtr<TypeName> result_type) {
  return ASTNew<Signature>(tok, std::move(arg_type_list), result_type);
}

ASTPointer Signature::Clone() const {
  return New(this->token, CloneASTVec<TypeName>(this->arg_type_list),
             this->result_type ? ASTCast<AST::TypeName>(this->result_type->Clone())
                               : nullptr);
}

Signature::Signature(Token tok, ASTVec<TypeName> arg_type_list,
                     ASTPtr<TypeName> result_type)
    : Base(ASTKind::Signature, tok),
      arg_type_list(std::move(arg_type_list)),
      result_type(result_type) {
}

} // namespace fire::AST
