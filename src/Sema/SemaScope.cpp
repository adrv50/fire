#include <list>

#include "alert.h"
#include "Error.h"
#include "Sema/Sema.h"

namespace fire::semantics_checker {

LocalVar::LocalVar(ASTPtr<AST::VarDef> vardef) {

  this->name = vardef->GetName();
  this->decl = vardef;
}

LocalVar::LocalVar(ASTPtr<AST::Argument> arg) {
  // Don't use this ctor if arg is template.

  this->name = arg->GetName();
  this->arg = arg;
}

// ------------------------------------
//  ScopeContext ( base-struct )

bool ScopeContext::Contains(ScopeContext* scope, bool recursive) const {

  switch (this->type) {
  case SC_Block: {
    auto block = (BlockScope*)scope;

    for (auto&& c : block->child_scopes) {
      if (c == scope)
        return true;

      if (recursive && c->Contains(scope, recursive))
        return true;
    }

    break;
  }

  case SC_Func: {
    auto func = (FunctionScope*)scope;

    return func->block == scope || (recursive && func->block->Contains(scope, recursive));
  }

  default:
    todo_impl;
  }

  return false;
}

ASTPointer ScopeContext::GetAST() const {
  alert;
  return nullptr;
}

LocalVar* ScopeContext::find_var(string_view const&) {
  alert;
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ASTPointer) {
  alert;
  return nullptr;
}

ScopeContext* ScopeContext::find_child_scope(ScopeContext*) {
  alert;
  return nullptr;
}

vector<ScopeContext*> ScopeContext::find_name(string const&) {
  alert;
  return {};
}

// ------------------------------------
//  BlockScope

BlockScope::BlockScope(int depth, ASTPtr<AST::Block> ast, int index_add)
    : ScopeContext(SC_Block),
      ast(ast) {

  ast->ScopeCtxPtr = this;

  this->depth = depth;

  if (!ast)
    return;

  for (auto&& e : ast->list) {
    switch (e->kind) {
    case ASTKind::Block: {
      this->AddScope(new BlockScope(this->depth + 1, ASTCast<AST::Block>(e)));
      break;
    }

    case ASTKind::Function:
      this->AddScope(new FunctionScope(this->depth + 1, ASTCast<AST::Function>(e)));
      break;

    case ASTKind::Vardef: {
      auto& v = this->add_var(ASTCast<AST::VarDef>(e));

      v.index_add = index_add + this->child_var_count;

      break;
    }

    case ASTKind::If: {
      auto d = e->As<AST::Statement>()->data_if;

      this->AddScope(new BlockScope(
          this->depth + 1, ASTCast<AST::Block>(ASTCast<AST::Block>(d->if_true))));

      if (d->if_false) {
        this->AddScope(new BlockScope(
            this->depth + 1, ASTCast<AST::Block>(ASTCast<AST::Block>(d->if_false))));
      }

      break;
    }

    case ASTKind::Match: {
      auto _match = ASTCast<AST::Match>(e);
      auto& ep = _match->patterns;

      for (auto&& eb : ep) {

        // for temporary variable definition
        auto eb_var_scope = new BlockScope(this->depth + 1, nullptr);

        eb_var_scope->ast = eb.block;

        if (!eb.everything) {
          ASTVec<Identifier> v;

          if (eb.expr->IsUnqualifiedIdentifier()) {
            v.emplace_back(ASTCast<AST::Identifier>(eb.expr));

            // eb.vardef_list.emplace_back(0, eb.expr->token.str);
          }
          else if (eb.expr->Is(ASTKind::CallFunc)) {
            for (size_t i = 0; auto&& arg : eb.expr->As<CallFunc>()->args) {
              if (arg->Is(ASTKind::Identifier) && !arg->IsQualifiedIdentifier()) {
                v.emplace_back(ASTCast<AST::Identifier>(arg));

                eb.vardef_list.emplace_back(i, arg->token.str);
              }

              i++;
            }
          }

          for (size_t i = 0; auto&& e : v) {
            auto& var = eb_var_scope->variables.emplace_back();

            var.name = e->GetName();

            var.depth = eb_var_scope->depth;
            var.index = i++;
          }
        }

        // auto eb_scope = new BlockScope(this->depth + 2, eb.block);

        eb_var_scope->AddScope(new BlockScope(this->depth + 2, eb.block));

        this->AddScope(eb_var_scope);

        //
        // match ... {
        //   Kind::A(a, b)    // eb_var_scope (v-stack for 'a', 'b')
        //     => {           //   eb_scope
        //     }
        // }
      }

      break;
    }

    case ASTKind::While: {
      this->AddScope(new BlockScope(this->depth + 1, e->as_stmt()->data_while->block));

      break;
    }

    case ASTKind::Switch:
      todo_impl;

    case ASTKind::TryCatch: {
      auto d = e->as_stmt()->data_try_catch;

      this->AddScope(new BlockScope(this->depth + 1, d->tryblock));

      for (auto&& c : d->catchers) {
        auto b = new BlockScope(this->depth + 1, c.catched);

        auto& lvar = b->variables.emplace_back();

        lvar.name = c.varname.str;
        lvar.depth = b->depth;

        this->AddScope(b);
      }

      break;
    }

    case ASTKind::Class: {
      auto x = ASTCast<AST::Class>(e);

      // Sema::GetInstance()->add_class(x);

      for (auto&& mf : x->member_functions)
        this->AddScope(new FunctionScope(this->depth + 1, mf));

      break;
    }

      // case ASTKind::Enum: {
      //   Sema::GetInstance()->add_enum(ASTCast<AST::Enum>(e));
      //   break;
      // }

    case ASTKind::Namespace: {
      auto block = ASTCast<AST::Block>(e);

      auto scope = new NamespaceScope(
          this->depth, block, index_add + this->variables.size() + this->child_var_count);

      auto c = (NamespaceScope*)this->AddScope(scope);

      if (scope == c) {
        this->child_var_count += c->child_var_count + c->variables.size();
      }
      // else {
      //   this->child_var_count += scope->variables.size();
      // }

      break;
    }
    }
  }

  ast->stack_size = this->child_var_count + this->variables.size();

  // alertexpr(var_count_total);
}

BlockScope::~BlockScope() {
  for (auto&& c : this->child_scopes)
    delete c;
}

ScopeContext*& BlockScope::AddScope(ScopeContext* scope) {

  // if (scope->is_block) {
  //   this->child_var_count += ((BlockScope*)scope)->variables.size();
  // }

  if (scope->type == SC_Namespace) {

    auto src = (NamespaceScope*)scope;

    for (auto&& c : this->child_scopes) {
      if (c->type != SC_Namespace)
        continue;

      auto dest = (NamespaceScope*)c;

      if (dest->name != src->name)
        continue;

      dest->_ast.emplace_back(src->ast);

      src->ast->ScopeCtxPtr = dest;

      auto index_add = this->child_var_count;

      for (auto&& v : src->variables) {
        if (dest->find_var(v.name))
          continue;

        v.index_add = index_add;
        this->child_var_count++;

        dest->variables.emplace_back(v);
      }

      //
      // Marge namespace if same name.
      //
      for (auto&& c2 : src->child_scopes) {
        dest->AddScope(c2);
      }

      src->child_scopes.clear();

      delete src;

      return c;
    }
  }

  scope->_owner = this;

  return this->child_scopes.emplace_back(scope);
}

LocalVar& BlockScope::add_var(ASTPtr<AST::VarDef> def) {
  LocalVar* pvar = this->find_var(def->GetName());

  if (!pvar) {
    pvar = &this->variables.emplace_back(def);

    pvar->index = this->variables.size() - 1;
    // pvar->index_add = this->child_var_count;
  }

  pvar->depth = this->depth;
  def->index = pvar->index;

  // pvar->index = def->index = this->variables.size() - 1;

  this->ast->stack_size++;

  // this->child_var_count++;

  return *pvar;
}

ASTPointer BlockScope::GetAST() const {
  return this->ast;
}

LocalVar* BlockScope::find_var(string_view const& name) {
  for (auto&& var : this->variables) {
    if (var.name == name)
      return &var;
  }

  return nullptr;
}

ScopeContext* BlockScope::find_child_scope(ASTPointer ast) {
  alert;

  for (auto&& c : this->child_scopes) {
    if (c->GetAST() == ast)
      return c;

    if (auto s = c->find_child_scope(ast); s)
      return s;
  }

  return nullptr;
}

ScopeContext* BlockScope::find_child_scope(ScopeContext* ctx) {
  for (auto&& c : this->child_scopes) {
    if (c == ctx)
      return c;

    if (auto s = c->find_child_scope(ctx); s)
      return s;
  }

  return nullptr;
}

vector<ScopeContext*> BlockScope::find_name(string const& name) {
  vector<ScopeContext*> vec;

  for (auto&& c : this->child_scopes) {
    if (c->IsNamedAs(name)) {
      vec.emplace_back(c);
    }
  }

  return vec;
}

std::string BlockScope::to_string() const {
  static int indent = 0;

  string s = "block depth=" + std::to_string(this->depth) + " {\n";

  int _indent = indent;
  indent++;

  for (auto&& x : this->child_scopes) {
    s += std::string(indent * 2, ' ') + x->to_string() + "\n";
  }

  indent = _indent;
  return s + "\n" + std::string(indent * 2, ' ') + "}";
}

// ------------------------------------
//  FunctionScope

FunctionScope::FunctionScope(int depth, ASTPtr<AST::Function> ast)
    : ScopeContext(SC_Func),
      ast(ast) {

  ast->ScopeCtxPtr = this;

  this->depth = depth;

  auto S = Sema::GetInstance();

  for (auto&& [_key, _val] : S->function_scope_map) {
    if (_key == ast) {
      todo_impl; // why again?
    }
  }

  // auto f = Sema::SemaFunction(ast);
  // f.scope = this;

  S->function_scope_map.emplace_back(ast, this);

  for (auto&& arg : ast->arguments) {
    this->add_arg(arg);
  }

  this->block = new BlockScope(this->depth + 1, ast->block);

  this->block->_owner = this;
}

FunctionScope::~FunctionScope() {
  delete this->block;
}

LocalVar& FunctionScope::add_arg(ASTPtr<AST::Argument> def) {
  LocalVar& arg = this->arguments.emplace_back(def->GetName());

  arg.arg = def;
  arg.is_argument = true;

  arg.depth = this->depth;
  arg.index = this->arguments.size() - 1;

  return arg;
}

ASTPointer FunctionScope::GetAST() const {
  return this->ast;
}

LocalVar* FunctionScope::find_var(string_view const& name) {
  for (auto&& arg : this->arguments) {
    if (arg.name == name)
      return &arg;
  }

  return nullptr;
}

ScopeContext* FunctionScope::find_child_scope(ASTPointer ast) {
  alert;

  if (this->ast == ast)
    return this;

  if (this->block->ast == ast)
    return this->block;

  return this->block->find_child_scope(ast);
}

ScopeContext* FunctionScope::find_child_scope(ScopeContext* ctx) {
  alert;

  if (this == ctx)
    return this;

  if (ctx == this->block)
    return this->block;

  return this->block->find_child_scope(ctx);
}

vector<ScopeContext*> FunctionScope::find_name(string const& name) {
  return this->block->find_name(name);
}

std::string FunctionScope::to_string() const {
  return "function depth=" + std::to_string(this->depth) + " {\n" +
         this->block->to_string() + "\n}";
}

// ------------------------------------
//  NamespaceScope

NamespaceScope::NamespaceScope(int depth, ASTPtr<AST::Block> ast, int index_add)
    : BlockScope(depth, ast, index_add),
      name(ast->token.str) {

  ast->ScopeCtxPtr = this;

  this->type = SC_Namespace;

  // this->child_var_count = this->variables.size();
}

NamespaceScope::~NamespaceScope() {
}

// ------------------------------------

} // namespace fire::semantics_checker