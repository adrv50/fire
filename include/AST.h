#pragma once

#include "alert.h"

#include "types.h"
#include "Token.h"

#include "AST/Kind.h"

#include "AST/Base/Base.h"
#include "AST/Base/Named.h"
#include "AST/Base/Templatable.h"

#include "AST/Expr.h"
#include "AST/Stmt.h"
#include "AST/Types.h"
#include "AST/Enum.h"
#include "AST/Class.h"
#include "AST/Function.h"

namespace fire::AST {

string ToString(ASTPointer ast);

ASTPtr<Identifier> GetID(ASTPointer);

char const* GetKindStr(ASTKind const);

} // namespace fire::AST