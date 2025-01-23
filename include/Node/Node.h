#pragma once

#include <functional>

#include "Sema_fwd.h"

#include "TypeInfo.h"
#include "Token/Token.h"

#include "defines.h"

namespace fire {

namespace Builtins {
struct BuiltinFunc;
}

enum NodeKind : u16 {
  ND_Value,

  ND_Identifier,
  ND_ScopeResol,

  ND_Variable,
  ND_Functor,
  ND_EnumeratorName,

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
  ND_DefEnumerator,                 // K
  ND_DefEnumeratorWithValue,        // K(T)
  ND_DefEnumeratorWithStructFields, // K(a: T, ...)

  ND_Struct,
  ND_StructMember,

  ND_Class,
  ND_Class_Fields,
  ND_Class_Methods,

  ND_Namespace,

  ND_Program,

  ND_TypeName,

  //
  //
  ND_TemplateArguments,
  ND_TemplateParameterList, // => { ND_Identifier* }

  //
  // Concept definition
  ND_Concept,
  ND_ConceptBody,

  ND_CCRule_ResultTypeExpection, // "(expr) => T"  # result type

  // Concept uses
  ND_ConceptTagsList, // [C1, C2, ...]
  ND_ConceptTag,      // C<...>
  ND_ConceptTagMulti, // (C1<T> or C2<T>)

  //
  // part of any nodes
  //
  ND_PairNameAndType, // "a: T"
  ND_InitializerList, // "{a: 1, b: 2, ...}"  (repeat of ND_PairNameAndType)
};

enum CompareExprKind : u8 {
  CMP_None,
  CMP_Bigger,
  CMP_BiggerOrEqual,
};

struct Token;
struct Node {
  NodeKind kind;
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

  Builtins::BuiltinFunc const* bfun = nullptr;

  fire::sema::NodeContext* sema_ctx = nullptr;
  fire::sema::Symbol* sym = nullptr;

  bool _is_type_evaluated = false;
  TypeInfo evaluated_type;

  bool is(NodeKind kind) const;

  bool is_id_or_sr() const;
  bool is_loop_stmt() const;

  bool is_named_node() const;

  string get_name() const;

  Node*& append(Node* node);

  Node* get_enumerator(size_t index) const;

  Node* clone();

  static Node* new_node(NodeKind kind, Token* tok = nullptr);

  static Node* new_node(NodeKind kind, Token* tok, Node* lhs, Node* rhs = nullptr);

  static Node* new_value(Token* tok, Object* obj);

  static Node* new_compare(CompareExprKind ck, Token* op, Node* lhs, Node* rhs);

  //
  // stop when func() returns true
  static bool walk_node(Node* nd, std::function<bool(Node*&)> const& func);

  Node(NodeKind kind, Token* tok, Object* obj = nullptr);
  Node(NodeKind kind, Token* tok, Node* lhs, Node* rhs);
  ~Node();

  Node* get_last_id();
};

} // namespace fire