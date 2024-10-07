#pragma once

#include "alert.h"

#include "types.h"
#include "Token.h"

#include "AST/Kind.h"
#include "AST/Base.h"
#include "AST/Expr.h"
#include "AST/Stmt.h"
#include "AST/Types.h"
#include "AST/Function.h"

namespace fire::AST {

string ToString(ASTPointer ast);

}