#pragma once

#include <functional>
#include "Builtins.h"

#define nd_items list
#define nd_elements list

#define nd_value obj

#define nd_array_elements list
#define nd_tuple_elements list

#define nd_dict_pairs list
#define nd_dict_pair_key na
#define nd_dict_pair_value nb

#define nd_variable_is_global b1
#define nd_variable_offset size

#define nd_id_name tok
#define nd_id_template_args list
#define nd_id_target nb
#define nd_id_enumerator_index size

//
// if name of enumerator needing initializers,
// pointer to call-func expression.
#define nd_id_enumerator_callctor nb // => ND_CallFunc

#define nd_scope_resol_first na
#define nd_scope_resol_idlist list

#define nd_callctor_ctor_side na
#define nd_callctor_initializers list

#define nd_callctor_init_key tok  // member-name
#define nd_callctor_init_value na // value

//
// ND_CallFunc
#define nd_callfunc_callee na
#define nd_callfunc_callee_userdef nb
#define nd_callfunc_callee_builtin bfun
#define nd_callfunc_is_method_call b1
#define nd_callfunc_method_self nc
#define nd_callfunc_args list
#define nd_callfunc_enum_ctor_enum nd
#define nd_callfunc_enum_ctor_index size

//
// ND_Cast
#define nd_cast_to_type na
#define nd_cast_from_expr nb

//
// ND_Range
#define nd_range_begin na
#define nd_range_end nb

//
// ND_Type
#define nd_type_is_mut b1
#define nd_type_is_ref b2
#define nd_type_template_args list

// if
#define nd_if_cond na
#define nd_if_then nb
#define nd_if_else nc

// switch
#define nd_switch_cond na
#define nd_switch_cases list
#define nd_switch_default_case nb

// switch-case
#define nd_switch_case_cond na
#define nd_switch_case_body nb

// match
#define nd_match_cond na
#define nd_match_cases list

// match-case
#define nd_match_case_cond na
#define nd_match_case_body nb

// loop
#define nd_loop_body na

//
// for loop
//
#define nd_for_init na
#define nd_for_cond nb
#define nd_for_step nc
#define nd_for_body nd

// for-range
#define nd_forrange_range na
#define nd_forrange_body nb

// for-each
#define nd_foreach_init na
#define nd_foreach_iter nb
#define nd_foreach_content nc
#define nd_foreach_cond nd
#define nd_foreach_step ne
#define nd_foreach_body nf

// while
#define nd_while_cond na
#define nd_while_body nb

// let
#define nd_let_name tok2
#define nd_let_type na
#define nd_let_init nb

#define nd_func_name tok2
#define nd_func_is_method b2
#define nd_func_is_template b3
#define nd_func_tplist nc // template parameters list
#define nd_func_cclist nd // concept tags (if used)
#define nd_func_args list
#define nd_func_result_type na
#define nd_func_is_variable_args b1
#define nd_func_body nb
#define nd_func_args_ti v1   // => Vec<TypeInfo>*
#define nd_func_result_ti v2 // => TypeInfo*

#define nd_func_arg_name tok
#define nd_func_arg_type na

#define nd_lhs na
#define nd_rhs nb

#define nd_return_expr na

#define nd_enum_name tok2
#define nd_enum_enumerators list

#define nd_enumerator_name tok
#define nd_enumerator_is_value b1
#define nd_enumerator_is_struct b2
#define nd_enumerator_val_type na
#define nd_enumerator_struct_members list

#define nd_class_name tok2
#define nd_class_fields na
#define nd_class_methods nb
#define nd_class_cclist nc

#define nd_struct_name tok2
#define nd_struct_members list
#define nd_struct_member_name tok
#define nd_struct_member_type na

#define nd_program_main na
#define nd_program_items list
#define nd_program_global_var_size size

#define nd_nametype_pair_name tok
#define nd_nametype_pair_type na

#define nd_concept_tags_list_list list

#define nd_concepttag_name tok

#define nd_concept_cclist na
#define nd_concept_name tok
#define nd_concept_parameters list
#define nd_concept_ccbody nb

#define nd_ccbody_rules list

enum NodeKind : u16 {
  ND_Value,

  ND_Identifier,
  ND_ScopeResol,
  ND_CallConstructor,
  ND_CallCtorPair,

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

  ND_ForwardInclement, // ++a
  ND_ForwardDecrement, // --a

  ND_BackwardInclement, // a++
  ND_BackwardDecrement, // a--

  ND_ConstructEnumeratorValue,
  ND_ConstructEnumeratorStruct,

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

  ND_In,

  ND_Or,
  ND_And,

  ND_Range,

  ND_ExprIf, // "A if B else c"

  ND_Assign,

  ND_Let,

  ND_If,
  ND_IfLet,
  ND_Switch,
  ND_SwitchCase,

  ND_Match,
  ND_MatchCase,

  //
  // for-loop
  ND_For,
  ND_ForEach,
  ND_ForRange,

  ND_Loop,
  ND_While,

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
  ND_Class_Fields,
  ND_Class_Methods,

  ND_Namespace,

  ND_Program,

  ND_TypeName,

  ND_TemplateParameterList, // <T, U, ...>
  ND_TemplateParam,         // T

  // Concept definition
  ND_Concept,

  // Concept uses
  ND_ConceptTagsList, // [C1, C2, ...]
  ND_ConceptTag,      // C1(T, U)
  ND_ConceptTagMulti, // (C1(T) or C2(T))

  //
  // part of any nodes
  //
  ND_PairNameAndType, // "a: T"
  ND_InitializerList, // "{a: 1, b: 2, ...}"  (repeat ND_PairNameAndType)
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

struct Token;
struct Node {
  NodeKind kind;
  NodeIdentifierKind id_kind = ID_None;
  CompareExprKind cmp_kind = CMP_None;
  Token* tok;
  Vec<Node*> list;

  Token* first_tok = nullptr;
  Token* last_tok = nullptr;

  TypeKind tk = TypeKind::None;

  Node* na = nullptr;
  Node* nb = nullptr;
  Node* nc = nullptr;
  Node* nd = nullptr;
  Node* ne = nullptr;
  Node* nf = nullptr;

  Object* obj = nullptr;
  Token* tok2 = nullptr;
  Token* tok3 = nullptr;
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
  bool b4 = false;

  void* v1 = nullptr;
  void* v2 = nullptr;
  void* v3 = nullptr;
  void* v4 = nullptr;

  size_t size = 0;
  size_t size2 = 0;

  Builtins::BuiltinFunc const* bfun;

  bool is(NodeKind kind) const;

  bool is_id_or_sr() const {
    return this->is(ND_Identifier) || this->is(ND_ScopeResol);
  }

  string get_name() const;

  Node*& append(Node* node);

  Node* get_enumerator(size_t index) const {
    return this->nd_enum_enumerators[index];
  }

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
  static bool walk_node(Node* nd, std::function<bool(Node*&)> const& func);

  Node(NodeKind kind, Token* tok, Object* obj = nullptr);
  Node(NodeKind kind, Token* tok, Node* lhs, Node* rhs);
  ~Node();

  Node* get_last_id() {
    if (this->is(ND_Identifier))
      return this;

    return this->nd_scope_resol_idlist.back();
  }
};