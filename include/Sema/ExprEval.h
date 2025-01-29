#pragma once

#include <functional>

#include "Node/Node.h"
#include "Token/Token.h"

#include "ScopeContext.h"
#include "NodeContext.h"

#include "Driver/Error.h"

namespace fire::sema {

struct ExprEvalContext {
  //
  //  関数の中にいる場合は True
  bool in_call_func = false;

  //
  //  関数呼び出し式の左辺 (呼び出し先) として評価されている場合は True
  bool as_functor = false;

  //
  //  関数呼び出し式の左辺 (呼び出し先) として評価されている場合はそのノード
  Node* callfunc_nd = nullptr;

  //
  //  関数呼び出し式の引数として評価されている場合はその引数の型情報
  Vec<TypeInfo>* callfunc_args_p = nullptr;

  //
  //  コンストラクタ呼び出し式の左辺として評価されている場合は True
  bool left_of_call_ctor_expr = false;

  //
  //  コンストラクタ呼び出し式全体のノード
  Node* callctor = nullptr;

  //
  //  コンストラクタ呼び出し式の左辺として評価されている場合はそのノード
  Node* callctor_left = nullptr;

  //
  //  空配列リテラルの許可フラグ
  //  要素の型がわかる文脈であれば True
  bool allowed_empty_array = false;

  //
  //  配列リテラルの型宣言ノード
  Node* array_type_decl = nullptr;

  //
  //  配列リテラルの要素の型
  TypeInfo* evaluated_array_type = nullptr;

  //
  //  match 文の case 文における比較対象として評価されている場合は True
  bool is_match_case_compare = false;

  //
  //  match 文の条件式の型
  TypeInfo* match_stmt_root_cond_type = nullptr;

  //
  //  match 文の条件式のノード
  Node* match_stmt_root_cond_node = nullptr;

  //
  //  未定義の識別子の許可フラグ
  bool allow_undefined_ident = false;

  //
  //  未定義の識別子の置換リスト
  Vec<pair<Node*, TypeInfo*>> unk_id_replaces{};

  //
  //  列挙子の引数なし呼び出しの許可フラグ
  bool allow_use_enumerator_without_args = false;

  //
  //  スコープ解決式の中にいる場合は True
  bool in_scope_resolution = false;

  //
  //  スコープ解決式の対象となるスコープ
  ScopeContext* sr_target_scope = nullptr;

  //
  //  メンバアクセスの右辺として評価されている場合は True
  bool in_right_of_member_access = false; // "a.b" --> b

  //
  //  メンバアクセス式全体のノード
  Node* mb_ac_node = nullptr;

  //
  //  メンバアクセス式の左辺として評価されている場合はそのノード
  Node* mb_ac_left_node = nullptr;

  //
  //  メンバアクセス式の左辺の型
  TypeInfo* mb_ac_evaluated_left_type = nullptr;
};

struct ExprEvalResult {
  bool fail;
  TypeInfo type;
  ExprEvalContext context;
  unique_ptr<Error> err;

  ExprEvalResult(TypeInfo const& type, ExprEvalContext const& context);
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

  //
  // <expr> "=>" { .. }
  //  ^^^^
  TypeInfo handle_match_case_cond(Node* node);

public:
  ExprEval(Sema& S);

  TypeInfo eval(Node* node);

  TypeInfo expect(Node* node, TypeInfo const& type);

  TypeInfo expect_enumerator_type(Node* node);

  TypeInfo expect_lvalue(Node* node);
  TypeInfo expect_lvalue(Node* node, TypeInfo const& _expect);

  TypeInfo make_type_from_symbol(Symbol* sym);

  TypeInfo operator()(Node* node) {
    return this->eval(node);
  }

  bool is_lvalue(Node* node);
};

} // namespace fire::sema