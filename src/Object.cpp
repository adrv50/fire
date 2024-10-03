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

i64 Object::get_vi() const {
  return this->is_int() ? this->as_primitive()->vi : 0;
}

double Object::get_vf() const {
  return this->is_float() ? this->as_primitive()->vf : 0;
}

char16_t Object::get_vc() const {
  return this->is_char() ? this->as_primitive()->vc : 0;
}

bool Object::get_vb() const {
  return this->is_boolean() ? this->as_primitive()->vb : 0;
}

ObjPrimitive* ObjPrimitive::to_float() {
  switch (this->type.kind) {
  case TypeKind::Int:
    this->vf = static_cast<double>(this->vi);
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
  std::string ret;

  for (auto it = this->list.begin(); it != this->list.end(); it++) {
    ret += (*it)->ToString();
    if (it < this->list.end() - 1)
      ret += ", ";
  }

  return "[" + ret + "]";
}

// ----------------------------
//  ObjString

ObjPointer ObjString::SubString(size_t pos, size_t length) {
  auto obj = ObjNew<ObjString>();

  for (size_t i = pos, end = pos + (length == 0 ? this->list.size() - pos : length);
       i < end; i++) {
    obj->Append(this->list[i]->Clone());
  }

  return obj;
}

std::string ObjString::ToString() const {
  std::u16string temp;

  for (auto&& c : this->list)
    temp.push_back(c->As<ObjPrimitive>()->vc);

  return utils::to_u8string(temp);
}

ObjPointer ObjString::Clone() const {
  auto obj = ObjNew<ObjString>();

  for (auto&& c : this->list) {
    obj->Append(c->Clone());
  }

  return obj;
}

ObjString::ObjString(std::u16string const& str)
    : ObjIterable(TypeKind::String) {
  for (auto&& c : str)
    this->Append(ObjNew<ObjPrimitive>(c));
}

ObjString::ObjString(std::string const& str)
    : ObjString(utils::to_u16string(str)) {
}

// ----------------------------
//  ObjEnumerator

std::string ObjEnumerator::ToString() const {
  return this->ast->GetName() +
         "::" + this->ast->enumerators->list[this->index]->token.str;
}

ObjEnumerator::ObjEnumerator(ASTPtr<AST::Enum> ast, int index)
    : Object(TypeKind::Enumerator),
      ast(ast),
      index(index) {
  this->type.name = this->ast->GetName();
}

// ----------------------------
//  ObjInstance

ObjPointer ObjInstance::Clone() const {
  auto obj = ObjNew<ObjInstance>(this->ast);

  for (auto&& m : this->member_variables) {
    obj->add_member_var(m->Clone());
  }

  return obj;
}

string ObjInstance::ToString() const {
  i64 _index = 0;

  auto const& mvarlist = this->ast->member_variables;

  return this->ast->GetName() + "{" +
         utils::join<ObjPointer>(", ", this->member_variables,
                                 [&mvarlist, &_index](ObjPointer obj) {
                                   return mvarlist[_index++]->GetName() + ": " +
                                          obj->ToString();
                                 }) +
         "}";
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

string ObjCallable::ToString() const {
  auto _args = this->type.params;

  _args.erase(_args.begin());

  return "<callable (" +
         utils::join<TypeInfo>(", ", _args,
                               [](TypeInfo const& t) {
                                 return t.to_string();
                               }) +
         ") -> " + this->type.params[0].to_string() + ">";
}

ObjCallable::ObjCallable(ASTPtr<AST::Function> fp)
    : Object(TypeKind::Function),
      func(fp),
      builtin(nullptr),
      is_named(true) {
  assert(fp);
}

ObjCallable::ObjCallable(builtins::Function const* fp)
    : Object(TypeKind::Function),
      func(nullptr),
      builtin(fp),
      is_named(true) {
  assert(fp);
}

// ----------------------------
//  ObjModule
std::string ObjModule::ToString() const {
  return "(ObjModule '" + this->name + "')";
}

// ----------------------------
//  ObjType

std::string ObjType::GetName() const {
  return this->ast_class ? this->ast_class->GetName() : this->ast_enum->GetName();
}

std::string ObjType::ToString() const {
  if (this->ast_class)
    return "<class '" + this->ast_class->GetName() + "'>";

  return "<enum '" + this->ast_enum->GetName() + "'>";
}

ObjType::ObjType(ASTPtr<AST::Enum> x)
    : Object(TypeKind::TypeName),
      ast_enum(x) {
  if (x)
    this->type.name = this->ast_enum->GetName();
}

ObjType::ObjType(ASTPtr<AST::Class> x)
    : Object(TypeKind::TypeName),
      ast_class(x) {
  this->type.name = this->ast_class->GetName();
}

// ----------------------------
//  ObjException

} // namespace fire