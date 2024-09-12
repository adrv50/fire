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

  ObjNone() : Object(TypeKind::None) {
  }
};

struct ObjPrimitive : Object {
  union {
    i64 vi;
    double vf;
    size_t vsize;
    bool vb;
    char16_t vc;

    u64 _data = 0;
  };

  ObjPrimitive* to_float();

  ObjPointer Clone() const override;
  std::string ToString() const override;

  ObjPrimitive(i64 vi) : Object(TypeKind::Int), vi(vi) {};

  ObjPrimitive(double vf) : Object(TypeKind::Float), vf(vf) {};

  ObjPrimitive(size_t vsize)
      : Object(TypeKind::Size), vsize(vsize) {};

  ObjPrimitive(bool vb) : Object(TypeKind::Bool), vb(vb) {};

  ObjPrimitive(char16_t vc) : Object(TypeKind::Char), vc(vc) {};
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

  ObjIterable(TypeInfo type) : Object(type) {
  }
};

// instance of class
struct ObjInstance : Object {
  ASTPtr<AST::Class> ast;
  builtin::Class const* builtin;

  std::map<std::string, ObjPointer> member;
  std::vector<ObjCallable> member_funcs;

  ObjPointer Clone() const override;
  std::string ToString() const override;

  ObjInstance(ASTPtr<AST::Class> ast);
  ObjInstance(builtin::Class const* ast);
};

struct ObjString : ObjIterable {
  ObjPointer SubString(size_t pos, size_t length);

  std::string ToString() const override;

  ObjString(std::u16string const& str = u"");
  ObjString(std::string const& str);
};

//
// TypeKind::Function
//
struct ObjCallable : Object {
  AST::Function const* func;
  builtin::Function const* builtin;

  ObjPointer Clone() const override;
  std::string ToString() const override;

  ObjCallable(AST::Function const* fp);
  ObjCallable(builtin::Function const* fp);
};

} // namespace metro