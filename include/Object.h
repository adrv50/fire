#pragma once

#include <concepts>
#include "Utils.h"
#include "fire-fwd.h"

#include "TypeInfo.h"

namespace fire {

namespace Builtins {
struct BuiltinFunc;
}

using Obj = Object*;

template <typename T>
using ObjPtr = T*;

struct Object {
  TypeInfo ti;
  bool is_marked;
  size_t ref_count;

  ObjNone* as_none();
  ObjInt* as_int();
  ObjFloat* as_float();
  ObjBool* as_bool();
  ObjChar* as_char();
  ObjStr* as_str();
  ObjVector* as_vector();
  ObjTuple* as_tuple();
  ObjDict* as_dict();
  ObjFunctor* as_functor();
  ObjEnumerator* as_enumerator();
  ObjInstance* as_instance();
  ObjTypeInfo* as_typeinfo();

  ObjNone const* as_none() const;
  ObjInt const* as_int() const;
  ObjFloat const* as_float() const;
  ObjBool const* as_bool() const;
  ObjChar const* as_char() const;
  ObjStr const* as_str() const;
  ObjVector const* as_vector() const;
  ObjTuple const* as_tuple() const;
  ObjDict const* as_dict() const;
  ObjFunctor const* as_functor() const;
  ObjEnumerator const* as_enumerator() const;
  ObjInstance const* as_instance() const;
  ObjTypeInfo const* as_typeinfo() const;

  virtual string to_string() const = 0;
  virtual Obj clone() const = 0;

  virtual bool equals(Obj obj) const = 0;

  virtual string to_string_as_element() const;

  static ObjNone* none;

  virtual ~Object() = default;

protected:
  Object(TypeInfo const& ti);
};

struct ObjNone : Object {
  ObjNone();

  string to_string() const override;
  ObjNone* clone() const override;

  bool equals(Obj obj) const override;

  static ObjNone* make();
};

struct ObjInt : Object {
  i64 val;

  ObjInt(i64 val);

  string to_string() const override;
  ObjInt* clone() const override;

  bool equals(Obj obj) const override;

  static ObjInt* make(i64 val);
};

struct ObjFloat : Object {
  f64 val;

  ObjFloat(f64 val);

  string to_string() const override;
  ObjFloat* clone() const override;

  bool equals(Obj obj) const override;

  static ObjFloat* make(f64 val);
};

struct ObjBool : Object {
  bool val;

  ObjBool(bool val);

  ObjBool* clone() const override;
  string to_string() const override;

  bool equals(Obj obj) const override;

  static ObjBool* make(bool val);
};

struct ObjChar : Object {
  char16_t val;

  ObjChar(char16_t val);

  string to_string() const override;
  ObjChar* clone() const override;

  string to_string_as_element() const override;

  bool equals(Obj obj) const override;

  static ObjChar* make(char16_t val);
};

struct ObjStr : Object {
  std::u16string val;

  ObjStr(std::u16string const& val);

  string to_string() const override;
  ObjStr* clone() const override;

  string to_string_as_element() const override;

  size_t length() const;

  bool equals(Obj obj) const override;

  static ObjStr* make(std::u16string const& val);
  static ObjStr* make(string const& val);
};

struct ObjVector : Object {
  Vec<Obj> list;

  ObjVector(TypeInfo const& elem_ti, Vec<Obj> const& val);

  string to_string() const override;
  ObjVector* clone() const override;

  bool equals(Obj obj) const override;

  static ObjVector* make(TypeInfo const& elem_ti, Vec<Obj> const& val = {});
};

struct ObjTuple : Object {
  Vec<Obj> list;

  ObjTuple(TypeInfo const& elem_ti, Vec<Obj> const& val);

  string to_string() const override;
  ObjTuple* clone() const override;

  bool equals(Obj obj) const override;

  static ObjTuple* make(TypeInfo const& elem_ti, Vec<Obj> const& val = {});
};

struct ObjDict : Object {
  TypeInfo const& key_ti;
  TypeInfo const& value_ti;

  Vec<pair<Obj, Obj>> list;

  ObjDict(TypeInfo const& key_ti, TypeInfo const& value_ti,
          Vec<pair<Obj, Obj>> const& data);

  string to_string() const override;
  ObjDict* clone() const override;

  bool equals(Obj obj) const override;

  static ObjDict* make(TypeInfo const& key_ti, TypeInfo const& value_ti,
                       Vec<pair<Obj, Obj>> const& data = {});
};

struct ObjFunctor : Object {
  Node* func;
  Builtins::BuiltinFunc const* bfun;

  ObjFunctor(Node* func);
  ObjFunctor(Builtins::BuiltinFunc const* bfun);

  string to_string() const override;
  ObjFunctor* clone() const override;

  bool equals(Obj obj) const override;

  static ObjFunctor* make(Node* func);
};

struct ObjEnumerator : Object {
  Node* nd_enum;
  size_t index;
  Vec<Obj> data;

  ObjEnumerator(Node* nd_enum, size_t index);

  string to_string() const override;
  ObjEnumerator* clone() const override;

  bool equals(Obj obj) const override;

  static ObjEnumerator* make(Node* nd_enum, size_t index);
};

struct ObjInstance : Object {
  Node* def;
  Vec<Obj> members;

  ObjInstance(Node* def, Vec<Obj> const& members);

  string to_string() const override;
  ObjInstance* clone() const override;

  bool equals(Obj obj) const override;

  static ObjInstance* make(Node* def, Vec<Obj> const& members = {});
};

struct ObjTypeInfo : Object {
  ObjTypeInfo(TypeInfo const& ti);

  string to_string() const override;
  ObjTypeInfo* clone() const override;

  bool equals(Obj obj) const override;

  static ObjTypeInfo* make(TypeInfo const& ti);
};

} // namespace fire