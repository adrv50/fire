#include "Sema/Sema.h"

namespace fire::semantics_checker {

SemaContext SemaContext::NullCtx;

static vector<Sema*> _sema_instances;

Sema* Sema::GetInstance() {
  return *_sema_instances.rbegin();
}

[[maybe_unused]]
static string scope2s_var(string indent, string ns, Vec<LocalVar> const& lvar) {
  string ret;

  indent += "  ";

  ret += indent + ns + "{\n";

  for (auto&& v : lvar) {
    ret +=
        indent + utils::Format("  '%.*s': decl=%p, distance=%d, index=%d, index_add=%d\n",
                               (int)v.name.length(), v.name.data(), v.decl.get(), v.depth,
                               v.index, v.index_add);
  }

  ret += indent + "},\n";

  return ret;
}

[[maybe_unused]]
static string scope2s(ScopeContext* scope) {
  static int _indent = 0;

  static char const* types[] = {
      "block", "func", "enum", "class", "namespace",
  };

  string indent = string(_indent * 2, ' ');

  if (!scope)
    return "(null)";

  string ret =
      indent + utils::Format("%s %p: {\n", types[static_cast<int>(scope->type)], scope);

  _indent++;

  ret += indent + utils::Format("  depth = %d,\n%s  ast = %p,\n%s  _owner = %p\n",
                                scope->depth, indent.c_str(), scope->GetAST().get(),
                                indent.c_str(), scope->_owner);

  switch (scope->type) {
  case ScopeContext::SC_Block:
  case ScopeContext::SC_Namespace: {
    auto block = (BlockScope*)scope;

    if (scope->type == ScopeContext::SC_Namespace)
      ret += indent + "  name: " + ((NamespaceScope*)scope)->name + "\n";

    ret += scope2s_var(indent, "variables: ", block->variables);

    for (auto&& c : block->child_scopes) {
      ret += scope2s(c) + "\n";
    }

    break;
  }

  case ScopeContext::SC_Func: {
    auto func = (FunctionScope*)scope;

    ret += scope2s_var(indent, "arguments: ", func->arguments);
    ret += scope2s(func->block) + "\n";

    break;
  }

  default: {
    break;
  }
  }

  _indent--;

  return ret + indent + "}";
}

Sema::Sema(ASTPtr<AST::Block> prg)
    : root(prg) {
  _sema_instances.emplace_back(this);

  this->_scope_context = new BlockScope(-1, AST::Block::New(""));
  this->_scope_context->AddScope(new BlockScope(0, prg));

  this->_scope_history.emplace_front(this->_scope_context);

  // debug(std::cout << scope2s(this->_scope_context) << std::endl);

  // this->GetScopeLoc() = {this->_scope_context, {this->_scope_context}};

  // std::cout << this->_scope_context->to_string() << std::endl;
  // todo_impl;
}

Sema::~Sema() {
  _sema_instances.pop_back();

  delete this->_scope_context;
}

} // namespace fire::semantics_checker