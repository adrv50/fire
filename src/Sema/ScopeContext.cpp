#include "Token/Token.h"
#include "Driver/Error.h"
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

bool ScopeContext::is_named() const {
  switch (this->kind) {
    case SC_Function:
    case SC_Enum:
    case SC_Class:
    case SC_Struct:
    case SC_Namespace:
      return true;
  }

  return false;
}

bool ScopeContext::get_name(string& out) const {
  switch (this->kind) {
    case SC_Function:
      out = this->node->nd_func_name->str;
      return true;

    case SC_Enum:
      out = this->node->nd_enum_name->str;
      return true;

    case SC_Class:
      out = this->node->nd_class_name->str;
      return true;

    case SC_Struct:
      out = this->node->nd_struct_name->str;
      return true;

    case SC_Namespace:
      out = this->node->nd_namespace_name->str;
      return true;
  }

  return false;
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

ScopeContext*& ScopeContext::append_as_symboled_scope(ScopeContext* scope,
                                                      SymbolKind kind, Node* sym_decl,
                                                      string const& name) {
  auto& c = this->childs.emplace_back(scope);

  c->parent = this;

  auto sym = this->add_symbol(new Symbol(kind, &this->sym_table));
  sym->decl = sym_decl;
  sym->name = name;
  sym->scope = scope;

  sym_decl->sym = sym;

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
        auto& sym = scope->add_symbol(new Symbol(SY_Var));

        sym->name = nd->nd_let_name->str;
        sym->decl = nd;

        nd->sema_ctx = new NodeContext();
        nd->sema_ctx->let_sym_ptr = sym;
        nd->sema_ctx->let_sym_ptr->var = scope->varlist.append(new VarInfo(sym));

        nd->sym = sym;

        break;
      }

      case ND_Block: {
        scope->append(ScopeContext::from_block(S, nd));
        break;
      }

      case ND_Function: {
        auto func = scope->append_as_symboled_scope(ScopeContext::from_function(S, nd),
                                                    SY_Func, nd, nd->nd_func_name->str);

        if (nd->nd_func_is_template) {
          func->add_template_params(nd->nd_func_tplist);

          func->func_ctx->template_ir = S.tp_manager.add_define(nd->sym);
        }

        break;
      }

      case ND_Enum: {
        scope->append_as_symboled_scope(ScopeContext::from_enum(S, nd), SY_Enum, nd,
                                        nd->nd_enum_name->str);

        break;
      }

      case ND_Struct: {
        todo_impl;
      }

      case ND_Class: {
        scope->append_as_symboled_scope(ScopeContext::from_class(S, nd), SY_Class, nd,
                                        nd->nd_class_name->str);

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

      case ND_If:
        scope->append(ScopeContext::from_block(S, nd->nd_if_then));

        if (nd->nd_if_else)
          scope->append(ScopeContext::from_block(S, nd->nd_if_else));

        break;
    }
  }

  return scope;
}

ScopeContext* ScopeContext::from_function(Sema& S, Node* node,
                                          ScopeContext* parent_class) {
  auto scope = new ScopeContext(SC_Function, node);

  scope->func_ctx = new FunctionContext();

  node->sema_ctx = new NodeContext();

  for (size_t i = 0; auto&& arg : node->nd_func_args) {
    auto& sym = scope->add_symbol(new Symbol(SY_Var));

    sym->name = arg->nd_func_arg_name->str;
    sym->decl = arg;

    sym->var = scope->varlist.append(new VarInfo(sym));
    sym->var->offset_in_stack = i++;
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

ScopeContext* ScopeContext::from_enum(Sema& S, Node* node) {
  auto scope = new ScopeContext(SC_Enum, node);

  auto ctx = new NodeContext();
  ctx->scope = scope;

  node->sema_ctx = ctx;

  for (auto&& en : node->nd_enum_enumerators) {
    auto sym = scope->add_symbol(new Symbol(SY_Enumerator, &scope->sym_table));

    sym->decl = en;
    sym->name = en->nd_enumerator_name->str;
    sym->scope = scope;
  }

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

void ScopeContext::add_template_params(Node* tplist) {

  for (auto&& param : tplist->list) {
    auto sym = this->add_symbol(new Symbol(SY_TemplateParam));

    sym->decl = param;
    sym->name = param->nd_id_name->str;
  }
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