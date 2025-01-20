#pragma once

#include <functional>

#include "Node/Node.h"
#include "SymbolTable.h"

namespace fire::sema {

namespace templates {
struct DefinitionIR;
}

struct SymbolInfo;
struct SymbolTable;
struct FunctionContext;

class Sema;

enum ScopeKind {
  SC_Block,

  SC_Function,

  SC_Enum,
  SC_Class,
  SC_Struct,

  // SC_Template, // => for symbols of template parameters. (and wrap template node)

  SC_Namespace,
};

struct VarInfo {
  TypeInfo type;
  Symbol* sym;

  size_t offset;          // => index for ScopeContext::varlist
  size_t offset_in_stack; // => index for FunctionContext::let_stmt_sym_ptr_list

  bool is_type_deducted;

  bool is_member = false;
  bool is_static_member = false;

  string const& get_name();

  VarInfo(Symbol* sym);
};

struct VarList {
  Vec<VarInfo*> list;
  ScopeContext* parent_scope;

  typename Vec<VarInfo*>::iterator begin();
  typename Vec<VarInfo*>::iterator end();

  VarInfo*& operator[](size_t index);

  VarInfo*& append(VarInfo* var);

  size_t size() const;

  VarInfo* find(string const& name);

  VarList(ScopeContext* parent_scope);
};

struct ScopeContext {

  ScopeKind kind;

  Node* node;

  SymbolTable sym_table;

  ScopeContext* parent;

  Vec<ScopeContext*> childs;

  VarList varlist;

  FunctionContext* func_ctx;

  bool is_named() const;

  bool get_name(string& out) const;

  bool contains(ScopeContext* child) const;

  Symbol*& add_symbol(Symbol* sym);

  ScopeContext*& append(ScopeContext* child);

  ScopeContext*& append_as_symboled_scope(ScopeContext* child, SymbolKind kind,
                                          Node* sym_decl, string const& name);

  size_t find_scope_if(Vec<ScopeContext*>& out, std::function<bool(ScopeContext*)> pred);

  size_t find_symbol_if(Vec<Symbol*>& out, std::function<bool(Symbol*)> pred);

  void add_template_params(Node* tplist);

  static ScopeContext* from_block(Sema& S, Node* node);

  static ScopeContext* from_function(Sema& S, Node* node,
                                     ScopeContext* parent_class = nullptr);

  static ScopeContext* from_enum(Sema& S, Node* node);
  static ScopeContext* from_struct(Sema& S, Node* node);
  static ScopeContext* from_class(Sema& S, Node* node);

  // static ScopeContext* from_namespace(Node* node);

  ScopeContext(ScopeKind kind, Node* node);
};

} // namespace fire::sema