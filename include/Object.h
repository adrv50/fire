#pragma once

#include <concepts>
#include <string>
#include <map>
#include "TypeInfo.h"

namespace fire {

namespace builtins {
struct Function;
}

struct Object {
  TypeInfo type;
  // i64 ref_count;
  bool is_marked;

  bool is_callable() const {
    return this->type.kind == TypeKind::Function;
  }

  bool is_numeric(bool contain_char = false) const {
    return this->type.is_numeric() ||
           (contain_char && this->type.kind == TypeKind::Char);
  }

  bool is_float() const {
    return this->type.kind == TypeKind::Float;
  }

  bool is_int() const {
    return this->type.kind == TypeKind::Int;
  }

  bool is_size() const {
    return this->type.kind == TypeKind::Size;
  }

  bool is_int_or_size() const {
    return this->is_int() || this->is_size();
  }

  bool is_integer() const {
    return this->is_int_or_size();
  }

  bool is_boolean() const {
    return this->type.kind == TypeKind::Bool;
  }

  bool is_instance() const {
    return this->type.kind == TypeKind::Instance;
  }

  bool is_string() const {
    return this->type.equals(TypeKind::String);
  }

  bool is_vector() const {
    return this->type.equals(TypeKind::Vector);
  }

  virtual ~Object() = default;

  virtual ObjPointer Clone() const = 0;
  virtual std::string ToString() const = 0;

  [[maybe_unused]]
  virtual bool Equals(ObjPointer obj) const {
    (void)obj;
    return false;
  }

  template <std::derived_from<Object> T>
  T* As() {
    return static_cast<T*>(this);
  }

  template <std::derived_from<Object> T>
  T const* As() const {
    return static_cast<T*>(this);
  }

protected:
  Object(TypeInfo type);
};

struct ObjNone : Object {
  ObjPointer Clone() const override {
    return ObjNew<ObjNone>();
  }

  std::string ToString() const override {
    return "none";
  }

  bool Equals(ObjPointer) const override {
    return true;
  }

  ObjNone()
      : Object(TypeKind::None) {
  }
};

struct ObjPrimitive : Object {
  union {
    i64 vi;
    double vf;
    size_t vsize;
    size_t vs;
    bool vb;
    char16_t vc;

    u64 _data = 0;
  };

  ObjPrimitive* to_float();

  ObjPointer Clone() const override;
  std::string ToString() const override;

  bool Equals(ObjPointer obj) const override {
    return this->type.equals(obj->type) &&
           this->_data == obj->As<ObjPrimitive>()->_data;
  }

  ObjPrimitive(i64 vi = 0)
      : Object(TypeKind::Int),
        vi(vi) {};

  ObjPrimitive(double vf)
      : Object(TypeKind::Float),
        vf(vf) {};

  ObjPrimitive(size_t vsize)
      : Object(TypeKind::Size),
        vsize(vsize) {};

  ObjPrimitive(bool vb)
      : Object(TypeKind::Bool),
        vb(vb) {};

  ObjPrimitive(char16_t vc)
      : Object(TypeKind::Char),
        vc(vc) {};
};

struct ObjIterable : Object {
  ObjVector list;

  ObjPointer& Append(ObjPointer obj) {
    return this->list.emplace_back(obj);
  }

  void AppendList(ObjPtr<ObjIterable> obj) {
    for (auto&& e : obj->list)
      this->Append(e->Clone());
  }

  size_t Count() const {
    return this->list.size();
  }

  ObjPointer Clone() const override;
  std::string ToString() const override;

  bool Equals(ObjPointer obj) const override {
    if (this->list.size() != obj->As<ObjIterable>()->list.size())
      return false;

    for (auto it = this->list.begin();
         auto&& e : obj->As<ObjIterable>()->list)
      if (!(*it++)->Equals(e))
        return false;

    return true;
  }

  ObjIterable(TypeInfo type)
      : Object(type) {
  }
};

struct ObjString : ObjIterable {
  ObjPointer SubString(size_t pos, size_t length = 0);

  size_t Length() const {
    return this->list.size();
  }

  std::string ToString() const override;

  ObjPointer Clone() const override {
    return PtrDynamicCast<ObjString>(this->ObjIterable::Clone());
  }

  ObjString(std::u16string const& str = u"");
  ObjString(std::string const& str);
};

//
// TypeKind::Enumerator
//
struct ObjEnumerator : Object {
  ASTPtr<AST::Enum> ast;
  int index;

  ObjPointer data = nullptr;

  ObjPointer Clone() const override {
    return ObjNew<ObjEnumerator>(*this);
  }

  std::string ToString() const override;

  bool Equals(ObjPointer obj) const override {
    return obj->type.equals(TypeKind::Enumerator) &&
           this->ast == obj->As<ObjEnumerator>()->ast &&
           this->index == obj->As<ObjEnumerator>()->index;
  }

  ObjEnumerator(ASTPtr<AST::Enum> ast, int index);
};

//
// instance of class
struct ObjInstance : Object {
  ASTPtr<AST::Class> ast;

  std::map<std::string, ObjPointer> member;
  std::vector<ObjPtr<ObjCallable>> member_funcs;

  ObjPointer& set_member_var(std::string const& name, ObjPointer obj);
  ObjPtr<ObjCallable>& add_member_func(ObjPtr<ObjCallable> obj);

  ObjPointer get_member_variable(std::string const& name) {
    if (this->member.contains(name))
      return this->member[name];

    return nullptr;
  }

  bool have_constructor() const;
  ASTPtr<AST::Function> get_constructor() const;

  ObjPointer Clone() const override;
  std::string ToString() const override;

  bool Equals(ObjPointer obj) const override {
    return this->ast == obj->As<ObjInstance>()->ast;
  }

  ObjInstance(ASTPtr<AST::Class> ast);
};

//
// ObjCallable
//
//  .func       = ptr to ast
//  .builtin    = ptr to builtin
//  .is_named   = when lambda this is false
//
struct ObjCallable : Object {
  ASTPtr<AST::Function> func;
  builtins::Function const* builtin;
  bool is_named = false;

  ObjPointer selfobj = nullptr;
  bool is_member_call = false;

  std::string GetName() const;

  ObjPointer Clone() const override;
  std::string ToString() const override;

  bool Equals(ObjPointer obj) const override {
    auto x = obj->As<ObjCallable>();

    return this->func == x->func && this->builtin == x->builtin &&
           this->is_named == x->is_named;
  }

  ObjCallable(ASTPtr<AST::Function> fp);
  ObjCallable(builtins::Function const* fp);
};

//
// TypeKind::Module
//
struct ObjModule : Object {
  std::string name;

  std::shared_ptr<SourceStorage> source;
  ASTPtr<AST::Block> ast;

  std::map<std::string, ObjPointer> variables;
  ObjVec<ObjType> types;
  ObjVec<ObjCallable> functions;

  ObjPointer Clone() const override {
    return ObjNew<ObjModule>(*this);
  }

  std::string ToString() const override;

  ObjModule(std::shared_ptr<SourceStorage> src,
            ASTPtr<AST::Block> ast = nullptr)
      : Object(TypeKind::Module),
        source(src),
        ast(ast) {
  }
};

//
// TypeKind::TypeName
//
struct ObjType : Object {
  TypeInfo typeinfo;

  ASTPtr<AST::Enum> ast_enum = nullptr;
  ASTPtr<AST::Class> ast_class = nullptr;

  ObjPointer Clone() const override {
    return ObjNew<ObjType>(*this);
  }

  std::string GetName() const;

  std::string ToString() const override;

  ObjType(TypeInfo type)
      : Object(TypeKind::TypeName),
        typeinfo(std::move(type)) {
  }

  ObjType(ASTPtr<AST::Enum> x = nullptr);
  ObjType(ASTPtr<AST::Class> x);
};

} // namespace fire