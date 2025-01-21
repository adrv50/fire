#pragma once

#include <functional>

#include "Node/Node.h"

namespace fire::sema::templates {

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

  Parameter(Node* decl);
};

struct ParamList {
  DefinitionIR* parent;
  Vec<Parameter> params;

  size_t size() const;
  Parameter& operator[](size_t i);
  Parameter const& operator[](size_t i) const;
  Parameter& push(Parameter p);
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

  Parameter* find_param(string const& name);
  bool compare_param_types(ParamList const& params);

  DefinitionIR(Symbol* definition);
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

  Instantiated(DefinitionIR* based);
};

class TemplateManager {

  friend class ::fire::sema::Sema;

  Vec<DefinitionIR*> definitions;

  Vec<Instantiated*> instantiated_templates;

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

} // namespace fire::sema::templates
