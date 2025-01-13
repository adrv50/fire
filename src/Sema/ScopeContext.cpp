#include "Token.h"
#include "Sema/Sema.h"

namespace fire::sema {

string const& VarInfo::get_name() {
  return this->sym->name;
}

VarInfo::VarInfo(Symbol* sym)
    : type(),
      sym(sym),
      offset(0),
      offset_in_stack(0),
      is_type_deducted(false) {
}

typename Vec<VarInfo*>::iterator VarList::begin() {
  return this->list.begin();
}

typename Vec<VarInfo*>::iterator VarList::end() {
  return this->list.end();
}

VarInfo*& VarList::operator[](size_t index) {
  return this->list[index];
}

VarInfo*& VarList::append(VarInfo* var) {
  return this->list.emplace_back(var);
}

size_t VarList::size() const {
  return this->list.size();
}

VarInfo* VarList::find(string const& name) {
  for (auto&& v : this->list)
    if (v->get_name() == name)
      return v;

  return nullptr;
}

VarList::VarList(ScopeContext* parent_scope)
    : list(),
      parent_scope(parent_scope) {
}

bool ScopeContext::contains(ScopeContext* child) const {
  return std::find(this->childs.begin(), this->childs.end(), child) != this->childs.end();
}

Symbol*& ScopeContext::add_symbol(Symbol* sym) {
  sym->parent_table = &this->sym_table;

  return this->sym_table.symbols.emplace_back(sym);
}

ScopeContext*& ScopeContext::append(ScopeContext* child) {
  auto& c = this->childs.emplace_back(child);

  c->parent = this;

  return c;
}

size_t ScopeContext::find_scope_if(Vec<ScopeContext*>& out,
                                   std::function<bool(ScopeContext*)> pred) {
  for (auto&& c : this->childs)
    if (pred(c))
      out.emplace_back(c);

  return out.size();
}

size_t ScopeContext::find_symbol_if(Vec<Symbol*>& out,
                                    std::function<bool(Symbol*)> pred) {
  for (auto&& sym : this->sym_table)
    if (pred(sym))
      out.emplace_back(sym);

  return out.size();
}

ScopeContext* ScopeContext::from_block(Sema& S, Node* node) {
  auto scope = new ScopeContext(SC_Block, node);

  node->sema_ctx = new NodeContext;

  node->sema_ctx->scope = scope;

  for (auto&& nd : node->nd_block_items) {
    switch (nd->kind) {
      case ND_Let: {
        auto& sym = scope->add_symbol(new Symbol(SY_Var, &scope->sym_table));

        sym->name = nd->nd_let_name->str;
        sym->decl = nd;

        nd->sema_ctx = new NodeContext();
        nd->sema_ctx->let_sym_ptr = sym;
        nd->sema_ctx->let_sym_ptr->var = scope->varlist.append(new VarInfo(sym));

        break;
      }

      case ND_Block: {
        scope->append(ScopeContext::from_block(S, nd));
        break;
      }

      case ND_Function: {
        scope->append(ScopeContext::from_function(S, nd));

        auto sym = scope->sym_table.push(new Symbol(SY_Func, &scope->sym_table));

        sym->decl = nd;
        sym->name = nd->nd_func_name->str;

        break;
      }

      case ND_Enum: {
        todo_impl;
      }

      case ND_Struct: {
        todo_impl;
      }

      case ND_Class: {
        auto cs = scope->append(ScopeContext::from_class(S, nd));

        auto sym = scope->sym_table.push(new Symbol(SY_Class, &scope->sym_table));

        sym->decl = nd;
        sym->name = nd->nd_class_name->str;
        sym->class_scope = cs;

        break;
      }

      case ND_Namespace: {
        auto ns = scope->append(ScopeContext::from_block(S, nd));

        ns->kind = SC_Namespace;

        auto sym = scope->add_symbol(new Symbol(SY_Namespace));

        sym->decl = nd;
        sym->name = nd->nd_namespace_name->str;
        sym->scope = ns;

        break;
      }
    }
  }

  return scope;
}

ScopeContext* ScopeContext::from_function(Sema& S, Node* node,
                                          ScopeContext* parent_class) {
  auto scope = new ScopeContext(SC_Function, node);

  scope->func_ctx = new FunctionContext();

  node->sema_ctx = new NodeContext();

  for (auto&& arg : node->nd_func_args) {
    auto& sym = scope->add_symbol(new Symbol(SY_Var));

    sym->name = arg->nd_func_arg_name->str;
    sym->decl = arg;

    sym->var = scope->varlist.append(new VarInfo(sym));
  }

  node->nd_func_lvar_count = node->nd_func_args.size();

  scope->append(ScopeContext::from_block(S, node->nd_func_body));

  Node::walk_node(node->nd_func_body, [&](Node* nd) -> bool {
    switch (nd->kind) {
      case ND_Return:
        scope->func_ctx->return_stmt_list.emplace_back(nd);
        break;

      case ND_Let:
        nd->sema_ctx->let_sym_ptr->var->offset_in_stack = nd->nd_let_offset =
            node->nd_func_lvar_count++;

        scope->func_ctx->let_stmt_sym_ptr_list.emplace_back(nd->sema_ctx->let_sym_ptr);

        break;
    }

    return false;
  });

  node->sema_ctx->func = scope->func_ctx;
  node->sema_ctx->scope = scope;

  return scope;
}

ScopeContext* ScopeContext::from_class(Sema& S, Node* node) {
  auto scope = new ScopeContext(SC_Class, node);

  auto ctx = new NodeContext();
  ctx->scope = scope;

  node->sema_ctx = ctx;

  for (auto&& member : node->nd_class_fields->list) {
    auto& sym = scope->sym_table.push(new Symbol(SY_Member, &scope->sym_table));

    sym->decl = member;
    sym->name = member->nd_let_name->str;
    sym->var = scope->varlist.append(new VarInfo(sym));

    member->sema_ctx = new NodeContext();
    member->sema_ctx->let_sym_ptr = sym;
  }

  for (auto&& method : node->nd_class_methods->list) {
    auto fn = scope->append(ScopeContext::from_function(S, method, scope));

    auto sym = scope->add_symbol(new Symbol(SY_Method));
    sym->decl = method;
    sym->name = method->nd_func_name->str;
  }

  return scope;
}

ScopeContext::ScopeContext(ScopeKind kind, Node* node)
    : kind(kind),
      node(node),
      sym_table(this),
      parent(nullptr),
      childs(),
      varlist(this),
      func_ctx(nullptr) {
}

} // namespace fire::sema