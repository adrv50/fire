#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using std::size_t;
using StringVector = std::vector<std::string>;

namespace metro {

template <class T>
using PShared = std::shared_ptr<T>;

template <class T>
using PUnique = std::unique_ptr<T>;

template <class T>
using PWeak = std::weak_ptr<T>;

template <class T, class U>
PShared<T> PtrCast(PShared<U> p) {
  return std::static_pointer_cast<T>(p);
}

enum class TypeKind : u8;
struct TypeInfo;

struct Object;
struct ObjPrimitive;
struct ObjCallable;
struct ObjIterable;
struct ObjInstance;

namespace value_type {

using Int = i64;
using Float = double;
using Size = u64;
using Char = char16_t;
using String = std::u16string;

} // namespace value_type

using ObjPointer = std::shared_ptr<Object>;
using ObjVector = std::vector<ObjPointer>;

template <class T>
using ObjPtr = std::shared_ptr<T>;

template <class T>
using ObjVec = std::vector<ObjPtr<T>>;

enum class TokenKind : u8;
struct Token;

using TokenVector = std::vector<Token>;
using TokenIterator = TokenVector::iterator;

struct SourceStorage;
struct SourceLocation;

namespace AST {
struct Base;
struct Value;
struct Variable;
struct Expr;
struct Block;
struct VarDef;
struct Statement;
struct TypeName;
struct Function;
struct Enum;
struct Class;
struct Templator;
struct Module;
struct Program;

} // namespace AST

using ASTPointer = std::shared_ptr<AST::Base>;
using ASTVector = std::vector<ASTPointer>;

namespace builtins {

struct Function;
struct Class;
struct Module;

} // namespace builtins

class Lexer;
class Parser;
class Evaluator;

namespace sema {
class Sema;
}

namespace gc {
class GC;
}

template <class T>
using ASTPtr = std::shared_ptr<T>;

template <class T>
using ASTVec = std::vector<std::shared_ptr<T>>;

template <class T, class... Args>
std::shared_ptr<T> ObjNew(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class T>
ASTPtr<T> ASTCast(ASTPointer p) {
  return std::static_pointer_cast<T>(p);
}

} // namespace metro