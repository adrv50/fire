#pragma once

#include <functional>

#include "../Node.h"
#include "SymbolTable.h"

namespace fire::sema {

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

  SC_Namespace,
};

struct VarInfo {
  TypeInfo type;
  Symbol* sym;
  size_t offset;
  bool is_type_deducted;

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

  bool contains(ScopeContext* child) const;

  ScopeContext*& append(ScopeContext* child);

  size_t find_scope_if(Vec<ScopeContext*>& out, std::function<bool(ScopeContext*)> pred);

  size_t find_symbol_if(Vec<Symbol*>& out, std::function<bool(Symbol*)> pred);

  static ScopeContext* from_block(Sema& S, Node* node);
  static ScopeContext* from_function(Sema& S, Node* node);

  ScopeContext(ScopeKind kind, Node* node);
};

} // namespace fire::sema