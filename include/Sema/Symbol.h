#pragma once

#include "../Node.h"

namespace fire::sema {

struct SymbolTable;
struct ScopeContext;

struct FunctionContext;

enum SymbolKind {
  SY_Var,  //
  SY_Func, // in global or namespace

  SY_Enum,       //
  SY_Enumerator, // enum

  SY_Struct,       //
  SY_StructMember, // struct

  SY_Class,        //
  SY_Method,       //
  SY_StaticMethod, //
  SY_Member,       //
  SY_StaticMember, // class

  SY_Namespace, // namespace
};

struct VarInfo;
struct Symbol {
  SymbolKind kind;

  string name;

  Node* decl;

  VarInfo* var;

  ScopeContext* scope;

  SymbolTable* parent_table;

  ScopeContext* get_scope() const;

  Symbol(SymbolKind kind, SymbolTable* table = nullptr);
};

} // namespace fire::sema