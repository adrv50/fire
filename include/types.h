#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#define _DBG_DONT_USE_SMART_PTR_ 0

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using std::size_t;

using std::string;
using std::string_view;

using std::vector;

template <typename T>
using Vec = std::vector<T>;

using StringVector = std::vector<std::string>;

static inline string operator+(string const& s, string_view sv) {
  return s + string(sv);
}

static inline string operator+(string_view sv, string const& s) {
  return string(sv) + s;
}

namespace fire {

enum class TypeKind : u8;
struct TypeInfo;

struct Object;
struct ObjPrimitive;
struct ObjIterable;
struct ObjString;
struct ObjEnumerator;
struct ObjInstance;
struct ObjCallable;
struct ObjModule;
struct ObjType;

namespace value_type {

using Int = i64;
using Float = double;
using Size = u64;
using Char = char16_t;
using String = std::u16string;

} // namespace value_type

enum class TokenKind : u8;
struct Token;

using TokenVector = std::vector<Token>;
using TokenIterator = TokenVector::iterator;

struct SourceStorage;
struct SourceLocation;

namespace AST {

struct Base;
struct Templatable;
struct Named;

struct TypeName;
struct Signature;

struct Value;
struct Identifier;
struct ScopeResol;

struct Array;
struct CallFunc;
struct Expr;

struct Block;
struct VarDef;
struct Statement;
struct Match;

struct Function;

struct Enum;
struct Class;

// struct Namespace;

} // namespace AST

namespace semantics_checker {
class Sema;
}

namespace builtins {

struct Function;
struct MemberVariable;

} // namespace builtins

#if _DBG_DONT_USE_SMART_PTR_
template <class T, class U>
T* PtrCast(U* p) {
  return reinterpret_cast<T*>(p);
}

template <class T, class U>
T* PtrDynamicCast(U* p) {
  return dynamic_cast<T*>(p);
}

using ObjPointer = Object*;

template <class T>
using ObjPtr = T*;

using ASTPointer = AST::Base*;

template <class T>
using ASTPtr = T*;

template <class T, class... Args>
T* ObjNew(Args&&... args) {
  return new T(std::forward<Args>(args)...);
}

template <class T>
ASTPtr<T> ASTCast(ASTPointer p) {
  return (T*)p;
}

#else

using ObjPointer = std::shared_ptr<Object>;

template <class T>
using ObjPtr = std::shared_ptr<T>;

using ASTPointer = std::shared_ptr<AST::Base>;

template <class T>
using ASTPtr = std::shared_ptr<T>;

template <class T, class U>
std::shared_ptr<T> PtrCast(std::shared_ptr<U> p) {
  return std::static_pointer_cast<T>(p);
}

template <class T, class U>
std::shared_ptr<T> PtrDynamicCast(std::shared_ptr<U> p) {
  return std::dynamic_pointer_cast<T>(p);
}

template <class T, class... Args>
inline std::shared_ptr<T> ObjNew(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class T>
ASTPtr<T> ASTCast(ASTPointer p) {
  return std::static_pointer_cast<T>(p);
}

#endif

using ObjVector = Vec<ObjPointer>;
using ASTVector = Vec<ASTPointer>;

template <class T>
using ObjVec = Vec<ObjPtr<T>>;

template <class T>
using ASTVec = Vec<ASTPtr<T>>;

template <typename T, typename U>
ASTVec<T> CloneASTVec(ASTVec<U> const& vec) {
  ASTVec<T> v;

  for (auto&& elem : vec)
    v.emplace_back(ASTCast<T>(elem->Clone()));

  return v;
}

} // namespace fire