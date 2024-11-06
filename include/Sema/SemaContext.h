#pragma once

#include "types.h"

namespace fire::semantics_checker {

class Sema;

struct SemaContext;
struct SemaFunctionNameContext;

struct SemaIdentifierEvalResult;

using TypeVec = Vec<TypeInfo>;

} // namespace fire::semantics_checker