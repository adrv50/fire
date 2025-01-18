#include "alert.h"
#include "Object.h"
#include "utf.h"
#include "Utils.h"
#include "Token.h"
#include "Node.h"

namespace fire {

template <std::derived_from<Object> T, typename... Args>
requires std::constructible_from<T, Args...>
ObjPtr<T> make_obj(Args&&... args) {
  return new T(std::forward<Args>(args)...);
}

ObjPtr<ObjNone> Object::none = make_obj<ObjNone>();

Object::Object(TypeInfo const& ti)
    : ti(ti),
      is_marked(false),
      ref_count(0) {
}

// ----------------------------------------
//  Cast wrappers
// ----------------------------------------
ObjNone* Object::as_none() {
  return this->ti.is(TypeKind::None) ? reinterpret_cast<ObjNone*>(this) : nullptr;
}

ObjInt* Object::as_int() {
  return reinterpret_cast<ObjInt*>(this);
}

ObjFloat* Object::as_float() {
  return this->ti.is(TypeKind::Float) ? reinterpret_cast<ObjFloat*>(this) : nullptr;
}

ObjBool* Object::as_bool() {
  return this->ti.is(TypeKind::Bool) ? reinterpret_cast<ObjBool*>(this) : nullptr;
}

ObjChar* Object::as_char() {
  return this->ti.is(TypeKind::Char) ? reinterpret_cast<ObjChar*>(this) : nullptr;
}

ObjStr* Object::as_str() {
  return this->ti.is(TypeKind::String) ? reinterpret_cast<ObjStr*>(this) : nullptr;
}

ObjVector* Object::as_vector() {
  return this->ti.is(TypeKind::Vector) ? reinterpret_cast<ObjVector*>(this) : nullptr;
}

ObjTuple* Object::as_tuple() {
  return this->ti.is(TypeKind::Tuple) ? reinterpret_cast<ObjTuple*>(this) : nullptr;
}

ObjDict* Object::as_dict() {
  return this->ti.is(TypeKind::Dict) ? reinterpret_cast<ObjDict*>(this) : nullptr;
}

ObjFunctor* Object::as_functor() {
  return this->ti.is(TypeKind::Functor) ? reinterpret_cast<ObjFunctor*>(this) : nullptr;
}

ObjEnumerator* Object::as_enumerator() {
  return this->ti.is(TypeKind::Enumerator) ? reinterpret_cast<ObjEnumerator*>(this)
                                           : nullptr;
}

ObjTypeInfo* Object::as_typeinfo() {
  return this->ti.is(TypeKind::Type) ? reinterpret_cast<ObjTypeInfo*>(this) : nullptr;
}

ObjNone const* Object::as_none() const {
  return this->ti.is(TypeKind::None) ? reinterpret_cast<ObjNone const*>(this) : nullptr;
}

ObjInt const* Object::as_int() const {
  return this->ti.is(TypeKind::Int) ? reinterpret_cast<ObjInt const*>(this) : nullptr;
}

ObjFloat const* Object::as_float() const {
  return this->ti.is(TypeKind::Float) ? reinterpret_cast<ObjFloat const*>(this) : nullptr;
}

ObjBool const* Object::as_bool() const {
  return this->ti.is(TypeKind::Bool) ? reinterpret_cast<ObjBool const*>(this) : nullptr;
}

ObjChar const* Object::as_char() const {
  return this->ti.is(TypeKind::Char) ? reinterpret_cast<ObjChar const*>(this) : nullptr;
}

ObjStr const* Object::as_str() const {
  return this->ti.is(TypeKind::String) ? reinterpret_cast<ObjStr const*>(this) : nullptr;
}

ObjVector const* Object::as_vector() const {
  return this->ti.is(TypeKind::Vector) ? reinterpret_cast<ObjVector const*>(this)
                                       : nullptr;
}

ObjTuple const* Object::as_tuple() const {
  return this->ti.is(TypeKind::Tuple) ? reinterpret_cast<ObjTuple const*>(this) : nullptr;
}

ObjDict const* Object::as_dict() const {
  return this->ti.is(TypeKind::Dict) ? reinterpret_cast<ObjDict const*>(this) : nullptr;
}

ObjFunctor const* Object::as_functor() const {
  return this->ti.is(TypeKind::Functor) ? reinterpret_cast<ObjFunctor const*>(this)
                                        : nullptr;
}

ObjEnumerator const* Object::as_enumerator() const {
  return this->ti.is(TypeKind::Enumerator) ? reinterpret_cast<ObjEnumerator const*>(this)
                                           : nullptr;
}

ObjTypeInfo const* Object::as_typeinfo() const {
  return this->ti.is(TypeKind::Type) ? reinterpret_cast<ObjTypeInfo const*>(this)
                                     : nullptr;
}

// ----------------------------------------
//  Constructors
// ----------------------------------------
ObjNone::ObjNone()
    : Object(TypeKind::None) {
}

ObjInt::ObjInt(i64 val)
    : Object(TypeKind::Int),
      val(val) {
}

ObjFloat::ObjFloat(f64 val)
    : Object(TypeKind::Float),
      val(val) {
}

ObjBool::ObjBool(bool val)
    : Object(TypeKind::Bool),
      val(val) {
}

ObjChar::ObjChar(char16_t val)
    : Object(TypeKind::Char),
      val(val) {
}

ObjStr::ObjStr(std::u16string const& val)
    : Object(TypeKind::String),
      val(val) {
}

ObjVector::ObjVector(TypeInfo const& elem_ti, Vec<Obj> const& val)
    : Object(TypeInfo(TypeKind::Vector, {elem_ti})),
      list(val) {
}

ObjTuple::ObjTuple(TypeInfo const& elem_ti, Vec<Obj> const& val)
    : Object(TypeInfo(TypeKind::Tuple, {elem_ti})),
      list(val) {
}

ObjDict::ObjDict(TypeInfo const& key_ti, TypeInfo const& value_ti,
                 Vec<pair<Obj, Obj>> const& val)
    : Object(TypeInfo(TypeKind::Dict, {key_ti, value_ti})),
      key_ti(this->ti.tp_args[0]),
      value_ti(this->ti.tp_args[1]),
      list(val) {
}

ObjFunctor::ObjFunctor(Node* func)
    : Object(TypeInfo(TypeKind::Functor)),
      func(func) {
}

ObjEnumerator::ObjEnumerator(Node* nd_enum, size_t index)
    : Object(TypeInfo(TypeKind::Enumerator).set_enum(nd_enum, index)),
      nd_enum(nd_enum),
      index(index) {
}

ObjTypeInfo::ObjTypeInfo(TypeInfo const& ti)
    : Object(ti) {
}

// ----------------------------------------
//  to_string
// ----------------------------------------
string Object::to_string_as_element() const {
  return this->to_string();
}

string ObjNone::to_string() const {
  return "none";
}

string ObjInt::to_string() const {
  return std::to_string(this->val);
}

string ObjFloat::to_string() const {
  return std::to_string(this->val);
}

string ObjBool::to_string() const {
  return this->val ? "true" : "false";
}

string ObjChar::to_string() const {
  return utf::to_utf8(std::u16string{1, this->val});
}

string ObjChar::to_string_as_element() const {
  return "\'" + this->to_string() + "\'";
}

string ObjStr::to_string() const {
  return utf::to_utf8(this->val);
}

string ObjStr::to_string_as_element() const {
  return "\"" + this->to_string() + "\"";
}

string ObjVector::to_string() const {
  return "[" +
         utils::join(", ", this->list,
                     [](Obj const& obj) -> string {
                       return obj->to_string();
                     }) +
         "]";
}

string ObjTuple::to_string() const {
  return "(" +
         utils::join(", ", this->list,
                     [](Obj const& obj) -> string {
                       return obj->to_string_as_element();
                     }) +
         ")";
}

string ObjDict::to_string() const {
  return "{" +
         utils::join(", ", this->list,
                     [](pair<Obj, Obj> const& item) -> string {
                       return item.first->to_string_as_element() + ": " +
                              item.second->to_string_as_element();
                     }) +
         "}";
}

string ObjFunctor::to_string() const {
  return "functor";
}

string ObjEnumerator::to_string() const {
  auto e = this->nd_enum;

  auto s = e->nd_enum_name->str +
           "::" + e->nd_enum_enumerators[this->index]->nd_enumerator_name->str;

  if (!this->data.empty())
    s += "(" +
         utils::join(", ", this->data,
                     [](Obj const& obj) -> string {
                       return obj->to_string_as_element();
                     }) +
         ")";

  return s;
}

string ObjTypeInfo::to_string() const {
  return "<typeinfo of " + this->ti.to_string() + ">";
}

// ----------------------------------------
//  clone
// ----------------------------------------
ObjNone* ObjNone::clone() const {
  return new ObjNone();
}

ObjInt* ObjInt::clone() const {
  return make_obj<ObjInt>(this->val);
}

ObjFloat* ObjFloat::clone() const {
  return make_obj<ObjFloat>(this->val);
}

ObjBool* ObjBool::clone() const {
  return make_obj<ObjBool>(this->val);
}

ObjChar* ObjChar::clone() const {
  return make_obj<ObjChar>(this->val);
}

ObjStr* ObjStr::clone() const {
  return make_obj<ObjStr>(this->val);
}

ObjVector* ObjVector::clone() const {
  Vec<Obj> cloned;

  for (auto&& item : this->list)
    cloned.emplace_back(item->clone());

  return make_obj<ObjVector>(this->ti, cloned);
}

ObjTuple* ObjTuple::clone() const {
  Vec<Obj> cloned;

  for (auto&& item : this->list)
    cloned.emplace_back(item->clone());

  return make_obj<ObjTuple>(this->ti, cloned);
}

ObjDict* ObjDict::clone() const {
  Vec<pair<Obj, Obj>> cloned;

  for (auto&& [key, value] : this->list)
    cloned.emplace_back(key->clone(), value->clone());

  return make_obj<ObjDict>(this->key_ti, this->value_ti, cloned);
}

ObjFunctor* ObjFunctor::clone() const {
  return make_obj<ObjFunctor>(this->func);
}

ObjEnumerator* ObjEnumerator::clone() const {
  return make_obj<ObjEnumerator>(this->nd_enum, this->index);
}

ObjTypeInfo* ObjTypeInfo::clone() const {
  return make_obj<ObjTypeInfo>(this->ti);
}

// ----------------------------------------
//  Constructor wrappers
// ----------------------------------------
ObjNone* ObjNone::make() {
  return make_obj<ObjNone>();
}

ObjInt* ObjInt::make(i64 val) {
  return make_obj<ObjInt>(val);
}

ObjFloat* ObjFloat::make(f64 val) {
  return make_obj<ObjFloat>(val);
}

ObjBool* ObjBool::make(bool val) {
  return make_obj<ObjBool>(val);
}

ObjChar* ObjChar::make(char16_t val) {
  return make_obj<ObjChar>(val);
}

ObjStr* ObjStr::make(std::u16string const& val) {
  return make_obj<ObjStr>(val);
}

ObjStr* ObjStr::make(string const& val) {
  return make_obj<ObjStr>(utf::to_utf16(val));
}

ObjVector* ObjVector::make(TypeInfo const& elem_ti, Vec<Obj> const& val) {
  return make_obj<ObjVector>(elem_ti, val);
}

ObjTuple* ObjTuple::make(TypeInfo const& elem_ti, Vec<Obj> const& val) {
  return make_obj<ObjTuple>(elem_ti, val);
}

ObjDict* ObjDict::make(TypeInfo const& key_ti, TypeInfo const& value_ti,
                       Vec<pair<Obj, Obj>> const& val) {
  return make_obj<ObjDict>(key_ti, value_ti, val);
}

ObjFunctor* ObjFunctor::make(Node* func) {
  return make_obj<ObjFunctor>(func);
}

ObjEnumerator* ObjEnumerator::make(Node* nd_enum, size_t index) {
  return make_obj<ObjEnumerator>(nd_enum, index);
}

ObjTypeInfo* ObjTypeInfo::make(TypeInfo const& ti) {
  return make_obj<ObjTypeInfo>(ti);
}

// ----------------------------------------
//  compare equality
// ----------------------------------------
bool ObjNone::equals(Obj obj) const {
  if (auto p = obj->as_none())
    return true;

  return false;
}

bool ObjInt::equals(Obj obj) const {
  if (auto p = obj->as_int())
    return this->val == p->val;

  return false;
}

bool ObjFloat::equals(Obj obj) const {
  if (auto p = obj->as_float())
    return this->val == p->val;

  return false;
}

bool ObjBool::equals(Obj obj) const {
  if (auto p = obj->as_bool())
    return this->val == p->val;

  return false;
}

bool ObjChar::equals(Obj obj) const {
  if (auto p = obj->as_char())
    return this->val == p->val;

  return false;
}

bool ObjStr::equals(Obj obj) const {
  if (auto p = obj->as_str())
    return this->val == p->val;

  return false;
}

bool ObjVector::equals(Obj obj) const {
  if (auto p = obj->as_vector()) {
    if (this->list.size() != p->list.size())
      return false;

    for (size_t i = 0; i < this->list.size(); i++)
      if (!this->list[i]->equals(p->list[i]))
        return false;

    return true;
  }

  return false;
}

bool ObjTuple::equals(Obj obj) const {
  if (auto p = obj->as_tuple()) {
    if (this->list.size() != p->list.size())
      return false;

    for (size_t i = 0; i < this->list.size(); i++)
      if (!this->list[i]->equals(p->list[i]))
        return false;

    return true;
  }

  return false;
}

bool ObjDict::equals(Obj obj) const {
  if (auto p = obj->as_dict()) {
    if (this->list.size() != p->list.size())
      return false;

    for (auto iter = this->list.begin(); auto&& [key, value] : p->list)
      if (!iter->first->equals(key) || !iter->second->equals(value))
        return false;
      else
        iter++;

    return true;
  }

  return false;
}

bool ObjFunctor::equals(Obj obj) const {
  if (auto p = obj->as_functor())
    return this->func == p->func;

  return false;
}

bool ObjEnumerator::equals(Obj obj) const {
  if (auto p = obj->as_enumerator())
    return this->nd_enum == p->nd_enum && this->index == p->index;

  return false;
}

bool ObjTypeInfo::equals(Obj obj) const {
  if (auto p = obj->as_typeinfo())
    return this->ti.equals(p->ti);

  return false;
}

// ----------------------------------------
//  ObjStr
// ----------------------------------------
size_t ObjStr::length() const {
  return this->val.size();
}

} // namespace fire