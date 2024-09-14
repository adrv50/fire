#pragma once

#include <concepts>
#include <string>
#include <map>
#include "TypeInfo.h"

namespace metro {

struct Object {
  TypeInfo type;
  // i64 ref_count;
  bool is_marked;

  bool is_callable() const {
    return this->type.kind == TypeKind::Function;
  }

  bool is_numeric() const {
    return this->type.is_numeric();
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

  virtual ~Object() = default;

  virtual ObjPointer Clone() const = 0;
  virtual std::string ToString() const = 0;

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

  ObjIterable(TypeInfo type)
      : Object(type) {
  }
};

struct ObjString : ObjIterable {
  ObjPointer SubString(size_t pos, size_t length);

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

  ObjPointer Clone() const override {
    return ObjNew<ObjEnumerator>(*this);
  }

  std::string ToString() const override;

  ObjEnumerator(ASTPtr<AST::Enum> ast, int index)
      : Object(TypeKind::Enumerator),
        ast(ast),
        index(index) {
  }
};

//
// instance of class
struct ObjInstance : Object {
  ASTPtr<AST::Class> ast;

  std::map<std::string, ObjPointer> member;
  std::vector<ObjPtr<ObjCallable>> member_funcs;

  ObjPointer& set_member_var(std::string const& name, ObjPointer obj);
  ObjPtr<ObjCallable>& add_member_func(ObjPtr<ObjCallable> obj);

  bool have_constructor() const;
  ASTPtr<AST::Function> get_constructor() const;

  ObjPointer Clone() const override;
  std::string ToString() const override;

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

  std::string GetName() const;

  ObjPointer Clone() const override;
  std::string ToString() const override;

  ObjCallable(ASTPtr<AST::Function> fp);
  ObjCallable(builtins::Function const* fp);
};

//
// TypeKind::Module
//
struct ObjModule : Object {
  std::unique_ptr<SourceStorage> source;
  ASTPtr<AST::Program> ast;

  ObjPointer Clone() const override {
    return ObjNew<ObjModule>(*this);
  }

  std::string ToString() const override;

  ObjModule()
      : Object(TypeKind::Module),
        ast(nullptr) {
  }

  ObjModule(ASTPtr<AST::Program> ast);
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

  std::string ToString() const override;

  ObjType()
      : Object(TypeKind::TypeName) {
  }
};

} // namespace metro