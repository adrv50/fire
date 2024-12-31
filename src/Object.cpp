#include "alert.h"
#include "Object.h"
#include "utf.h"
#include "Utils.h"

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
ObjInt* Object::as_int() {
  return reinterpret_cast<ObjInt*>(this);
}

ObjFloat* Object::as_float() {
  return reinterpret_cast<ObjFloat*>(this);
}

ObjBool* Object::as_bool() {
  return reinterpret_cast<ObjBool*>(this);
}

ObjStr* Object::as_str() {
  return reinterpret_cast<ObjStr*>(this);
}

ObjVector* Object::as_vector() {
  return reinterpret_cast<ObjVector*>(this);
}

ObjTuple* Object::as_tuple() {
  return reinterpret_cast<ObjTuple*>(this);
}

ObjDict* Object::as_dict() {
  return reinterpret_cast<ObjDict*>(this);
}

ObjInt const* Object::as_int() const {
  return reinterpret_cast<ObjInt const*>(this);
}

ObjFloat const* Object::as_float() const {
  return reinterpret_cast<ObjFloat const*>(this);
}

ObjBool const* Object::as_bool() const {
  return reinterpret_cast<ObjBool const*>(this);
}

ObjStr const* Object::as_str() const {
  return reinterpret_cast<ObjStr const*>(this);
}

ObjVector const* Object::as_vector() const {
  return reinterpret_cast<ObjVector const*>(this);
}

ObjTuple const* Object::as_tuple() const {
  return reinterpret_cast<ObjTuple const*>(this);
}

ObjDict const* Object::as_dict() const {
  return reinterpret_cast<ObjDict const*>(this);
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
      key_ti(this->ti.template_args[0]),
      value_ti(this->ti.template_args[1]),
      list(val) {
}

// ----------------------------------------
//  to_string
// ----------------------------------------
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

string ObjStr::to_string() const {
  return utf::to_utf8(this->val);
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
                       return obj->to_string();
                     }) +
         ")";
}

string ObjDict::to_string() const {
  return "{" +
         utils::join(", ", this->list,
                     [](pair<Obj, Obj> const& item) -> string {
                       return item.first->to_string() + ": " + item.second->to_string();
                     }) +
         "}";
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

// ----------------------------------------
//  ObjStr
// ----------------------------------------
size_t ObjStr::length() const {
  return this->val.size();
}
