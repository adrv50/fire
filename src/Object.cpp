#include <cassert>

#include "alert.h"
#include "GC.h"
#include "Utils.h"
#include "Builtin.h"

namespace metro {

Object::Object(TypeInfo type)
    : type(std::move(type)), is_marked(false) {
}

ObjPointer ObjPrimitive::Clone() const {
  return ObjNew<ObjPrimitive>(*this);
}

std::string ObjPrimitive::ToString() const {
  switch (this->type.kind) {
  case TypeKind::Int:
    return std::to_string(this->vi);

  case TypeKind::Float:
    return std::to_string(this->vf);

  case TypeKind::Size:
    return std::to_string(this->vsize);

  case TypeKind::Bool:
    return this->vb ? "true" : "false";

  case TypeKind::Char:
    return utils::to_u8string((char16_t[]){this->vc, 0});
  }

  return "(primitive-type-obj)";
}

ObjPointer ObjIterable::Clone() const {
  auto obj = ObjNew<ObjIterable>(this->type);

  for (auto&& x : this->list)
    obj->Append(x->Clone());

  return obj;
}

std::string ObjIterable::ToString() const {
  return "(obj-iterable)";
}

ObjPointer ObjString::SubString(size_t pos, size_t length) {
  auto obj = ObjNew<ObjString>();

  for (size_t i = pos, end = pos + length; i < end; i++) {
    obj->Append(this->list[i]->Clone());
  }

  return obj;
}

ObjString::ObjString(std::u16string const& str)
    : ObjIterable(TypeKind::String) {
  if (!str.empty())
    for (auto&& c : str)
      this->Append(ObjNew<ObjPrimitive>(c));
}

ObjString::ObjString(std::string const& str)
    : ObjString(utils::to_u16string(str)) {
}

ObjPointer ObjCallable::Clone() const {
  return ObjNew<ObjCallable>(*this);
}

std::string ObjCallable::ToString() const {
  return "(callable-object)";
}

ObjPointer ObjCallable::Call(ObjVector args) const {
  if (this->func) {
    todo_impl;
  }

  return this->builtin->Call(std::move(args));
}

ObjCallable::ObjCallable(AST::Function const* fp)
    : Object(TypeKind::Function), func(fp), builtin(nullptr) {
}

ObjCallable::ObjCallable(builtin::Function const* fp)
    : Object(TypeKind::Function), func(nullptr), builtin(fp) {
}

} // namespace metro