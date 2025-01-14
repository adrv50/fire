#pragma once

#include <functional>

#include "Node.h"
#include "Token.h"

#include "ScopeContext.h"
#include "NodeContext.h"

namespace fire::sema {

enum ExprEvalStatus {
  EX_Succeed,

  EX_Error,
};

struct ExprEvalContext {
  bool in_call_func = false;
  Node* callfunc_nd = nullptr;
  Vec<TypeInfo>* callfunc_args_p = nullptr;

  bool allowed_empty_array = false;
  Node* array_type_decl = nullptr;
  TypeInfo* evaluated_array_type = nullptr;

  bool in_scope_resolution = false;
  ScopeContext* sr_target_scope = nullptr;
};

class Sema;
class ExprEval {
  friend class Sema;

  Sema& S;

  ExprEvalContext ctx;

  Vec<ExprEvalContext> _saves;

  void save();
  void restore();

  void reset();

public:
  ExprEval(Sema& S);

  TypeInfo eval(Node* node);

  TypeInfo expect(Node* node, TypeInfo const& type);

  TypeInfo make_type_from_symbol(Symbol* sym);

  TypeInfo operator()(Node* node) {
    return this->eval(node);
  }
};

namespace templates {

struct Parameter;
struct Instantiated;
struct DefinitionIR;
struct Instantiated;
class TemplateManager;

//
// テンプレートパラメータ
// <T, U, ...>
struct Parameter {
  string name;
  Node* decl; // => ND_Identifier (in tplist)
  Symbol* sym;
  TypeInfo type;
  bool is_deducted = false;

  Parameter(Node* decl)
      : name(decl->nd_id_name->str),
        decl(decl),
        sym(decl->sym),
        type(),
        is_deducted(false) {
  }
};

struct ParamList {
  DefinitionIR* parent;
  Vec<Parameter> params;

  size_t size() const {
    return this->params.size();
  }

  Parameter& operator[](size_t i) {
    return this->params[i];
  }

  Parameter const& operator[](size_t i) const {
    return this->params[i];
  }

  Parameter& push(Parameter p) {
    return this->params.emplace_back(std::move(p));
  }

  //
  // テンプレート引数から型を取得する
  // 関数呼び出しの場合はその引数も見る
  void subtitute(Node* id, Vec<TypeInfo> const& args,
                 Vec<TypeInfo> const* callfunc_args = nullptr, Node* cf_expr = nullptr);

  Parameter* find(string const& name);

  //
  // if function
  Parameter* get_param_of_arg(size_t index);

  ParamList(DefinitionIR* parent);
};

//
// テンプレート関数・クラスの定義（中間表現）
struct DefinitionIR {
  Symbol* sym; // => symbol of definition
  Node* node;
  ParamList param_list;
  Vec<Instantiated*> instantiated_list;

  Parameter* find_param(string const& name) {
    for (auto&& p : this->param_list.params)
      if (p.name == name)
        return &p;

    return nullptr;
  }

  bool compare_param_types(ParamList const& params) {
    if (this->param_list.size() != params.size())
      return false;

    for (size_t i = 0; i < this->param_list.size(); i++) {
      auto& self = this->param_list[i];
      auto& p = params[i];

      if (!self.is_deducted || self.name != p.name)
        return false;

      if (!self.type.equals(p.type))
        return false;
    }

    return true;
  }

  DefinitionIR(Symbol* definition)
      : sym(definition),
        node(definition->decl),
        param_list(this) {
  }
};

//
// 実体
struct Instantiated {
  Symbol* sym;
  Node* node; // <= Replaced all parameter names
  DefinitionIR* based;
  ParamList params;

  Vec<TypeInfo> take_args;    //
  Vec<TypeInfo> take_cf_args; //

  Instantiated(DefinitionIR* based)
      : sym(based->sym),
        node(based->node),
        based(based),
        params(based->param_list /* copy */) {
  }
};

class TemplateManager {

  friend class Sema;

  Vec<DefinitionIR*> definitions;

  Vec<Instantiated*> instantiations;

public:
  TemplateManager();

  DefinitionIR* find_ir_from_sym(Symbol* sym);

  Instantiated* find_instantiated(Symbol* sym, Vec<TypeInfo> const& tp_args,
                                  Vec<TypeInfo> const& cf_args = {});

  Instantiated* instantiate(DefinitionIR* ir, Node* id, Vec<TypeInfo> const& tp_args,
                            Vec<TypeInfo> const& cf_args = {}, Node* cf_expr = nullptr);

  DefinitionIR* add_define(Symbol* sym);

  Node* replace_all_params(Instantiated* inst, Node* _node);

private:
};

} // namespace templates

class Sema {
  friend struct Symbol;
  friend struct SymbolTable;
  friend struct ScopeContext;

  friend class ExprEval;

  Node* program;

  ScopeContext* root_scope;

  ScopeContext* cur_scope;

  ExprEval expr_eval;

  templates::TemplateManager tp_manager;

  ScopeContext* enter_scope(ScopeContext* scope);
  void leave_scope();

  ScopeContext* find_scope(std::function<bool(ScopeContext*)> pred) {
    auto s = this->cur_scope;

    while (s && !pred(s))
      s = s->parent;

    return s;
  }

  ScopeContext* get_cur_func_scope() {
    return this->find_scope([](ScopeContext* s) {
      return s->kind == SC_Function;
    });
  }

  ScopeContext* get_cur_class_scope() {
    return this->find_scope([](ScopeContext* s) {
      return s->kind == SC_Class;
    });
  }

public:
  Sema(Node* program);

  Sema(Sema&&) = delete;
  Sema(Sema const&) = delete;

  void check_all();

  void check_top_item(Node* node);

  void check_class(Node* node);

  void check_func(Node* node, Node* parent_class = nullptr);

  void check_let(Node* node, Node* parent_class = nullptr);

  void check_stmt(Node* node);

  TypeInfo eval_type_ti(Node* node);

private:
  //
  // find_name:
  //   find in scope chain (current to root)
  size_t find_name(Vec<Symbol*>& out, string const& name, ScopeContext* start = nullptr);
};

} // namespace fire::sema