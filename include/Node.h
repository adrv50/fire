#pragma once

#include <functional>
#include "Builtins.h"

#define nd_value nd.obj

#define nd_array_elements list
#define nd_tuple_elements list

#define nd_dict_pairs nd.list
#define nd_dict_pair_key nd.na
#define nd_dict_pair_value nd.nb

#define nd_variable_is_global nd.b1
#define nd_variable_offset nd.size

#define nd_id_name tok
#define nd_id_template_args list
#define nd_id_target nd.na

//
// ND_CallFunc
#define nd_callfunc_callee nd.na
#define nd_callfunc_callee_userdef nd.nb
#define nd_callfunc_callee_builtin nd.bfun
#define nd_callfunc_is_method_call nd.b1
#define nd_callfunc_method_self nd.nc
#define nd_callfunc_args list

#define nd_type_is_mut nd.b1
#define nd_type_is_ref nd.b2
#define nd_type_template_args list

#define nd_items list
#define nd_elements list

// if
#define nd_if_cond nd.na
#define nd_if_then nd.nb
#define nd_if_else nd.nc

// while
#define nd_while_cond nd.na
#define nd_while_body nd.nb

// loop
#define nd_loop_body nd.na

// let
#define nd_let_name nd.tok2
#define nd_let_type nd.na
#define nd_let_init nd.nb

#define nd_func_name nd.tok2
#define nd_func_is_method nd.b2
#define nd_func_args list
#define nd_func_result_type nd.na
#define nd_func_is_variable_args nd.b1
#define nd_func_body nd.nb

#define nd_func_arg_name tok
#define nd_func_arg_type nd.na

#define nd_lhs nd.na
#define nd_rhs nd.nb

#define nd_scope_resol_first nd.na
#define nd_scope_resol_idlist list

#define nd_return_expr nd.na

#define nd_enum_name nd.tok2
#define nd_enum_enumerators list

#define nd_enumerator_name tok
#define nd_enumerator_is_value nd.b1
#define nd_enumerator_is_struct nd.b2
#define nd_enumerator_val_type nd.na
#define nd_enumerator_struct_members list

#define nd_class_name nd.tok2
#define nd_class_fields nd.na  // --> ND_Let
#define nd_class_methods nd.nb // --> ND_Function

#define nd_struct_name nd.tok2
#define nd_struct_members list
#define nd_struct_member_name tok
#define nd_struct_member_type nd.na

#define nd_program_main nd.na
#define nd_program_items list
#define nd_program_global_var_size nd.size

struct Token;

enum NodeKind : u16 {
  ND_Value,
  ND_Identifier,
  ND_ScopeResol,

  ND_Array,
  ND_Tuple,
  ND_Dict,
  ND_DictPair,

  ND_Not,
  ND_Ref,
  ND_Cast,

  ND_Subscript,
  ND_MemberAccess,
  ND_CallFunc,

  ND_Mul,
  ND_Div,
  ND_Mod,

  ND_Add,
  ND_Sub,

  ND_LShift,
  ND_RShift,

  //
  // A > B
  // A >= B
  ND_Compare,

  ND_Equal,
  // ND_NotEqual,

  ND_BitAnd,
  ND_BitOr,
  ND_BitXor,

  ND_Or,
  ND_And,

  ND_Assign,

  ND_Let,
  ND_If,
  ND_Match,

  ND_While,
  ND_For,
  ND_DoWhile,
  ND_Loop,

  ND_Break,
  ND_Continue,
  ND_Return,

  ND_Block,

  ND_Function,
  ND_FunctionArg,

  ND_Enum,
  ND_DefEnumerator,
  ND_DefEnumeratorStructFields,

  ND_Struct,
  ND_StructMember,

  ND_Class,

  ND_TypeName,

  ND_Program,
};

// -------------------------------------
//  NodeIdentifierKind:
//    The kind of identifier decided in Sema.
// -------------------------------------
enum NodeIdentifierKind : u8 { // for id or scope-resol
  ID_None,
  ID_Var,
  ID_Func,
  ID_Enum,
  ID_Enumerator,
  ID_Struct,
  ID_Class,
  ID_Namespace,
};

enum CompareExprKind : u8 {
  CMP_None,
  CMP_Bigger,
  CMP_BiggerOrEqual,
};

struct Node {
  NodeKind kind;
  NodeIdentifierKind id_kind = ID_None;
  CompareExprKind cmp_kind = CMP_None;
  Token* tok;
  Vec<Node*> list;

  union {
    void* __data[10]{0};

    struct {
      Node* ln;
      Node* rn;
      TypeKind tk;
    };

    struct {
      Node* na;
      Node* nb;
      Node* nc;
      Object* obj;
      Token* tok2;
      Token* tok3;
      bool b1;
      bool b2;
      bool b3;
    };

    struct {
      Node* nx;
      Node* ny;
      size_t size;
      Builtins::BuiltinFunc const* bfun;
    };
  } nd;

  bool is(NodeKind kind) const;

  string get_name() const;

  Node*& append(Node* node);

  static Node* new_node(NodeKind kind, Token* tok);

  static Node* new_node(NodeKind kind, Token* tok, Node* lhs, Node* rhs = nullptr);

  static Node* new_value(Token* tok, Object* obj);

  static Node* new_compare(CompareExprKind ck, Token* op, Node* lhs, Node* rhs) {
    auto nd = Node::new_node(ND_Compare, op, lhs, rhs);

    nd->cmp_kind = ck;

    return nd;
  }

  //
  // stop when func() returns true
  static bool walk_node(Node* nd, std::function<bool(Node*)> const& func);

  Node(NodeKind kind, Token* tok, Object* obj = nullptr);
  Node(NodeKind kind, Token* tok, Node* lhs, Node* rhs);
  ~Node();

  Node* get_last_id() {
    if (this->is(ND_Identifier))
      return this;

    return this->nd_scope_resol_idlist.back();
  }
};