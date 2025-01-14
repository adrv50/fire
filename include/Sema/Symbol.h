#pragma once

#include "../Node.h"

namespace fire::sema {

namespace templates {
struct Parameter;
}

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

  SY_TemplateParam,

  SY_Namespace, // namespace

  SY_BuiltinType,
  SY_BuiltinFunc,
};

struct VarInfo;
struct Symbol {
  SymbolKind kind;

  string name;

  Node* decl;

  union {
    VarInfo* var;
    templates::Parameter* tp_param;
    ScopeContext* scope;
    TypeKind tk;
    Builtins::BuiltinFunc const* bfun;
  };

  SymbolTable* parent_table;

  ScopeContext* get_scope() const;

  Symbol(SymbolKind kind, SymbolTable* table = nullptr);
};

} // namespace fire::sema