#pragma once

#include "../Node.h"

namespace fire::sema {

struct SymbolTable;
struct ScopeContext;

enum SymbolKind {
  SY_Unknown,

  SY_Var,
  SY_Func,

  SY_Enum,
  SY_Struct,
  SY_Class,

  SY_Namespace,
};

struct VarInfo;
struct Symbol {
  SymbolKind kind;

  string name;

  Node* decl;

  VarInfo* var;

  SymbolTable* parent_table;

  ScopeContext* get_scope() const;

  Symbol(SymbolKind kind, SymbolTable* table);
};

} // namespace fire::sema