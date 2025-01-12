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

ScopeContext*& ScopeContext::append(ScopeContext* child) {
  return this->childs.emplace_back(child);
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
        auto& sym = scope->sym_table.push(new Symbol(SY_Var, &scope->sym_table));

        sym->name = nd->nd_let_name->str;
        sym->decl = nd;

        break;
      }

      case ND_Block: {
        scope->append(ScopeContext::from_block(S, nd));
        break;
      }

      case ND_Function: {
        scope->append(ScopeContext::from_function(S, nd));
        break;
      }

      case ND_Enum: {
        todo_impl;
      }

      case ND_Struct: {
        todo_impl;
      }

      case ND_Class: {
        todo_impl;
      }
    }
  }

  return scope;
}

ScopeContext* ScopeContext::from_function(Sema& S, Node* node) {
  auto scope = new ScopeContext(SC_Function, node);

  node->sema_ctx = new NodeContext();

  for (auto&& arg : node->nd_func_args) {
    auto& sym = scope->sym_table.push(new Symbol(SY_Var, &scope->sym_table));

    sym->name = arg->nd_func_arg_name->str;
    sym->decl = arg;

    sym->var = scope->varlist.append(new VarInfo(sym));

    sym->var->type = S.eval_type_ti(arg->nd_func_arg_type);
    sym->var->is_type_deducted = true;
  }

  scope->append(ScopeContext::from_block(S, node->nd_func_body));

  scope->func_ctx = new FunctionContext();

  Node::walk_node(node->nd_func_body, [&](Node* nd) -> bool {
    if (nd->is(ND_Return))
      scope->func_ctx->return_stmt_list.emplace_back(nd);

    return false;
  });

  node->sema_ctx->func = scope->func_ctx;
  node->sema_ctx->scope = scope;

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