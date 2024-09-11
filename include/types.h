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

namespace metro {

enum class TypeKind : u8;
struct TypeInfo;

struct Object;
struct ObjPrimitive;
struct ObjCallable;
struct ObjIterable;
struct ObjInstance;

using ObjPointer = std::shared_ptr<Object>;
using ObjVector = std::vector<ObjPointer>;

template <class T>
using ObjPtr = std::shared_ptr<T>;

template <class T>
using ObjVec = std::vector<ObjPtr<T>>;

template <class T, class... Args>
std::shared_ptr<T> ObjNew(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

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

using ASTPointer = std::shared_ptr<Base>;
using ASTVector = std::vector<ASTPointer>;
} // namespace AST

template <class T>
using ASTPtr = std::shared_ptr<T>;

template <class T>
ASTPtr<T> ASTCast(AST::ASTPointer p) {
  return std::static_pointer_cast<T>(p);
}

template <class T>
using ASTVec = std::vector<std::shared_ptr<T>>;

namespace builtin {

struct Function;
struct Class;
struct Module;

} // namespace builtin

class Lexer;
class Parser;
class Compiler;
class Interpreter;

namespace sema {
class Sema;
}

namespace gc {
class GC;
}

} // namespace metro