#pragma once

#include <concepts>
#include "TypeInfo.h"
#include "utf.h"

struct Object;
struct ObjNone;
struct ObjInt;
struct ObjFloat;
struct ObjBool;
struct ObjStr;
struct ObjArray;
struct ObjTuple;
struct ObjDict;
struct ObjFunctor;

struct Node;

using Obj = Object*;

template <typename T>
using ObjPtr = T*;

struct Object {
  TypeInfo ti;
  bool is_marked;
  size_t ref_count;

  ObjInt* as_int();
  ObjFloat* as_float();
  ObjBool* as_bool();
  ObjStr* as_str();

  ObjInt const* as_int() const;
  ObjFloat const* as_float() const;
  ObjBool const* as_bool() const;
  ObjStr const* as_str() const;

  virtual string to_string() const = 0;
  virtual Obj clone() const = 0;

  static ObjNone* none;

  virtual ~Object() = default;

protected:
  Object(TypeInfo const& ti);
};

struct ObjNone : Object {
  ObjNone();

  string to_string() const override;
  ObjNone* clone() const override;

  static ObjNone* make();
};

struct ObjInt : Object {
  i64 val;

  ObjInt(i64 val);

  string to_string() const override;
  ObjInt* clone() const override;

  static ObjInt* make(i64 val);
};

struct ObjFloat : Object {
  f64 val;

  ObjFloat(f64 val);

  string to_string() const override;
  ObjFloat* clone() const override;

  static ObjFloat* make(f64 val);
};

struct ObjBool : Object {
  bool val;

  ObjBool(bool val);

  ObjBool* clone() const override;
  string to_string() const override;

  static ObjBool* make(bool val);
};

struct ObjChar : Object {
  char16_t val;

  ObjChar(char16_t val);

  string to_string() const override;
  ObjChar* clone() const override;

  static ObjChar* make(char16_t val);
};

struct ObjStr : Object {
  std::u16string val;

  ObjStr(std::u16string const& val);

  string to_string() const override;
  ObjStr* clone() const override;

  size_t length() const;

  static ObjStr* make(std::u16string const& val);
  static ObjStr* make(string const& val);
};

struct ObjArray : Object {
  Vec<Obj> val;

  ObjArray(Vec<Obj> const& val);

  string to_string() const override;
  ObjArray* clone() const override;

  static ObjArray* make(Vec<Obj> const& val = {});
};

struct ObjTuple : Object {
  Vec<Obj> val;

  ObjTuple(Vec<Obj> const& val);

  string to_string() const override;
  ObjTuple* clone() const override;

  static ObjTuple* make(Vec<Obj> const& val = {});
};

struct ObjDict : Object {
  Vec<pair<Obj, Obj>> data;

  ObjDict(Vec<pair<Obj, Obj>> const& data);

  string to_string() const override;
  ObjDict* clone() const override;

  static ObjDict* make(Vec<pair<Obj, Obj>> const& data = {});
};

struct ObjFunctor : Object {
  Node* func;

  ObjFunctor(Node* func);

  string to_string() const override;
  ObjFunctor* clone() const override;

  static ObjFunctor* make(Node* func);
};
