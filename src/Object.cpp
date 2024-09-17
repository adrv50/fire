#include <cassert>

#include "alert.h"
#include "Utils.h"
#include "Builtin.h"
#include "AST.h"

using namespace std::string_literals;

namespace fire {

Object::Object(TypeInfo type)
    : type(std::move(type)),
      is_marked(false) {
}

ObjPrimitive* ObjPrimitive::to_float() {
  switch (this->type.kind) {
  case TypeKind::Int:
    this->vf = static_cast<double>(this->vi);
    break;

  case TypeKind::Size:
    this->vf = static_cast<double>(this->vsize);
    break;

  default:
    return nullptr;
  }

  return this;
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

  todo_impl;
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

// ----------------------------
//  ObjString

ObjPointer ObjString::SubString(size_t pos, size_t length) {
  auto obj = ObjNew<ObjString>();

  for (size_t i = pos, end = pos + length; i < end; i++) {
    obj->Append(this->list[i]->Clone());
  }

  return obj;
}

std::string ObjString::ToString() const {
  std::u16string temp;

  for (auto&& c : this->list)
    temp.push_back(c->As<ObjPrimitive>()->vc);

  return utils::to_u8string(temp + u'\0');
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

// ----------------------------
//  ObjEnumerator

std::string ObjEnumerator::ToString() const {
  return this->ast->GetName() + "." +
         this->ast->enumerators[this->index].str;
}

ObjEnumerator::ObjEnumerator(ASTPtr<AST::Enum> ast, int index)
    : Object(TypeKind::Enumerator),
      ast(ast),
      index(index) {
  this->type.name = this->ast->GetName();
}

// ----------------------------
//  ObjInstance

ObjPointer& ObjInstance::set_member_var(std::string const& name,
                                        ObjPointer obj) {
  return this->member[name] = obj;
}

ObjPtr<ObjCallable>&
ObjInstance::add_member_func(ObjPtr<ObjCallable> obj) {
  return this->member_funcs.emplace_back(obj);
}

bool ObjInstance::have_constructor() const {
  return (bool)this->ast->constructor;
}

ASTPtr<AST::Function> ObjInstance::get_constructor() const {
  return this->ast->constructor;
}

ObjPointer ObjInstance::Clone() const {
  auto obj = ObjNew<ObjInstance>(this->ast);

  for (auto&& [key, val] : this->member) {
    obj->member[key] = val->Clone();
  }

  for (auto&& mf : this->member_funcs) {
    obj->member_funcs.emplace_back(PtrCast<ObjCallable>(mf->Clone()));
  }

  return obj;
}

std::string ObjInstance::ToString() const {
  return "(ObjInstance of '"s + this->ast->GetName() + "')";
}

ObjInstance::ObjInstance(ASTPtr<AST::Class> ast)
    : Object(TypeKind::Instance),
      ast(ast) {
  this->type.name = ast->GetName();
}

// ----------------------------
//  ObjCallable

std::string ObjCallable::GetName() const {
  if (this->is_named) {
    return this->func ? this->func->GetName() : this->builtin->name;
  }

  return "";
}

ObjPointer ObjCallable::Clone() const {
  return ObjNew<ObjCallable>(*this);
}

std::string ObjCallable::ToString() const {
  return "(ObjCallable)";
}

ObjCallable::ObjCallable(ASTPtr<AST::Function> fp)
    : Object(TypeKind::Function),
      func(fp),
      builtin(nullptr),
      is_named(true) {
}

ObjCallable::ObjCallable(builtins::Function const* fp)
    : Object(TypeKind::Function),
      func(nullptr),
      builtin(fp),
      is_named(true) {
}

// ----------------------------
//  ObjModule
std::string ObjModule::ToString() const {
  return "(ObjModule '" + this->name + "')";
}

// ----------------------------
//  ObjType

std::string ObjType::GetName() const {
  return this->ast_class ? this->ast_class->GetName()
                         : this->ast_enum->GetName();
}

std::string ObjType::ToString() const {
  return "(ObjType)";
}

ObjType::ObjType(ASTPtr<AST::Enum> x)
    : Object(TypeKind::TypeName),
      ast_enum(x) {
  this->type.name = this->ast_enum->GetName();
}

ObjType::ObjType(ASTPtr<AST::Class> x)
    : Object(TypeKind::TypeName),
      ast_class(x) {
  this->type.name = this->ast_class->GetName();
}

} // namespace fire