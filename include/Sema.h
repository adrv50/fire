#pragma once

#include <functional>
#include "Node.h"
#include "Builtins.h"

//
// for Sema::Scope::*
#define fnscope_ret_type ti
#define fnscope_args variables

class Sema {

  struct VarInfo {
    string name;
    TypeInfo ti;

    Node* decl = nullptr;

    size_t index = 0;
    bool is_type_deducted = false;

    TypeInfo& set_type(TypeInfo const& ti) {
      this->is_type_deducted = true;

      return this->ti = ti;
    }

    VarInfo();
    VarInfo(string const& name, TypeInfo const& ti = TypeKind::None);
  };

  struct VarList {
    Vec<VarInfo> variables;

    auto begin() {
      return this->variables.begin();
    }

    auto end() {
      return this->variables.end();
    }

    size_t size() const {
      return this->variables.size();
    }

    VarInfo& operator[](size_t index);

    VarInfo& append(VarInfo const& var);

    VarInfo* find(string const& name);

    VarList();
    VarList(Vec<VarInfo> const& variables);
  };

  enum ScopeType {
    SC_Global,

    SC_Enum,
    SC_Struct,
    SC_Class,

    SC_Function,
  };

  /*
    function scope
      - node = ND_Function
      - variables = arguments
      - ti = function return type
      - is_named = true

    block scope
      - node = ND_Block
      - variables = local variables
      - functions = functions defined in this block
  */

  struct Scope {

    ScopeType type;
    Scope* parent = nullptr;
    Vec<Scope*> childs;

    Node* node;

    VarList variables;
    Vec<Scope*> functions;

    VarInfo* letvp = nullptr;

    TypeInfo ti;

    Vec<TypeInfo> arg_types;
    Vec<Node*> ret_stmt_list;

    bool is_named;

    Scope*& append(Scope* scope) {
      scope->parent = this;

      return this->childs.emplace_back(scope);
    }

    Scope*& append_func(Scope* scope) {
      scope->parent = this;

      return this->functions.emplace_back(scope);
    }

    Scope* find(Node* node) {
      for (auto&& child : this->childs)
        if (child->node == node)
          return child;

      return nullptr;
    }

    string get_name() const;

    VarInfo* find_var(string const& name);

    Scope* find_func(string const& name);

    static Scope* make_scope(Node* node);

    Scope(ScopeType type, Node* node);
  };

  struct SemaContext {
    Scope* cur_scope;

    bool is_in_func = false;

    Scope* cur_func = nullptr;

    Scope* enter(Node* node);
    void leave();

    SemaContext(Scope* scope = nullptr)
        : cur_scope(scope) {
    }
  };

  Node* root;

  Scope* root_scope;

  SemaContext ctx;

  Scope* enter_func(Node* func);
  void leave_func(Node* func);

public:
  Sema(Node* root);

  void check_full();

  void check_func(Node* func);

  void check_stmt(Node* stmt);
  void check_let(Node* let);
  void check_block(Node* block);

  TypeInfo eval_expr_ti(Node* node);

  TypeInfo eval_type_ti(Node* nd_type);

  TypeInfo check_function_call(Node* call);

  void compare_call_arguments(Node* cf, bool is_method, Vec<TypeInfo> const& call,
                              Vec<TypeInfo> const& func, bool is_variable_args,
                              Node* fn, Builtins::BuiltinFunc const* bfn);

  bool is_method(Node* func);
  bool is_method(Builtins::BuiltinFunc const* bf);

private:
  struct NameFindResult {
    enum NameType {
      NA_NotFound,

      //
      // NA_Var:
      //   .var = VarInfo
      //   .scope = the scope where the variable is defined in.
      NA_Var,

      //
      // NA_Func:
      //   .func = Node (ND_Function)
      //   .scope = function scope
      NA_Func,

      NA_Enum,
      NA_Class,
      NA_Struct,

      NA_Namespace,
    };

    string name;

    Scope* scope;
    NameType type;

    VarInfo* var = nullptr;
    Node* func = nullptr;

    NameFindResult(string const& name, Scope* scope = nullptr,
                   NameType type = NA_NotFound)
        : name(name),
          scope(scope),
          type(type) {
    }

    bool is_found() const {
      return this->scope != nullptr;
    }
  };

  Scope*& get_cur_scope();

  Scope* find_scope_if(std::function<bool(Scope*)> const& pred,
                       Scope* from_this = nullptr, bool from_root = false,
                       bool reverse = false);

  NameFindResult scope_resolution(Node* sr, Scope* scope = nullptr);

  NameFindResult find_name(Node* id, Scope* from_this = nullptr,
                           bool from_root = false, bool reverse = false);

  NameFindResult find_name_wrap(Node* name, Scope* scope = nullptr) {
    return name->is(ND_ScopeResol) ? this->scope_resolution(name, scope)
                                   : this->find_name(name, scope);
  }

  static inline Scope* _cur_func_keep = nullptr;
};
