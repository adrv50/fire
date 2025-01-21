#pragma once

#include <functional>

#include "Node/Node.h"
#include "Token/Token.h"

#include "ScopeContext.h"
#include "NodeContext.h"

namespace fire::sema {

struct ExprEvalContext {
  bool in_call_func = false;
  bool as_functor = false;
  Node* callfunc_nd = nullptr;
  Vec<TypeInfo>* callfunc_args_p = nullptr;

  bool left_of_call_ctor_expr = false;
  Node* callctor = nullptr;
  Node* callctor_left = nullptr;

  bool allowed_empty_array = false;
  Node* array_type_decl = nullptr;
  TypeInfo* evaluated_array_type = nullptr;

  bool is_match_case_compare = false;
  TypeInfo* match_stmt_root_cond_type = nullptr;

  bool allow_undefined_ident = false;
  TypeInfo* unk_id_replace = nullptr;

  bool allow_use_enumerator_without_args = false;

  bool in_scope_resolution = false;
  ScopeContext* sr_target_scope = nullptr;

  bool in_right_of_member_access = false; // "a.b" --> b
  Node* mb_ac_node = nullptr;
  Node* mb_ac_left_node = nullptr;
  TypeInfo* mb_ac_evaluated_left_type = nullptr;
};

class Sema;
class ExprEval {
  friend class Sema;

  Sema& S;

  ExprEvalContext ctx;
  size_t ctx_patch_counter = 0;

  Vec<ExprEvalContext> _saves;

  void save();
  void restore();

  ExprEvalContext& get_saved(size_t dist = 0) {
    return this->_saves[this->_saves.size() - dist];
  }

  void reset();

  TypeInfo handle_call_constructor(Node* node);

  TypeInfo handle_construct_enumerator(TypeInfo const& enumerator_ti, Node* node);
  TypeInfo handle_construct_struct(TypeInfo const& struct_ti, Node* node);
  TypeInfo handle_construct_class(TypeInfo const& class_ti, Node* node);

public:
  ExprEval(Sema& S);

  TypeInfo eval(Node* node);

  TypeInfo expect(Node* node, TypeInfo const& type);

  TypeInfo expect_enumerator_type(Node* node);

  TypeInfo make_type_from_symbol(Symbol* sym);

  TypeInfo operator()(Node* node) {
    return this->eval(node);
  }
};

} // namespace fire::sema