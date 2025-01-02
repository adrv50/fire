#pragma once

#include "typedef.h"

enum class TypeKind : u8;
struct TypeInfo;

struct Object;
struct ObjNone;
struct ObjInt;
struct ObjFloat;
struct ObjBool;
struct ObjChar;
struct ObjStr;
struct ObjVector;
struct ObjTuple;
struct ObjDict;
struct ObjFunctor;
struct ObjEnumerator;
struct ObjInstance;
struct ObjTypeInfo;

enum class TokenKind : u8;
enum class TokenOperatorKind : u16;
enum class TokenPunctKind : u8;
enum class TokenKwdKind : u16;
struct Token;

enum NodeKind : u16;
enum NodeIdentifierKind : u8;
enum CompareExprKind : u8;
struct Node;

class Lexer;
class Parser;
class Sema;
class Evaluator;
