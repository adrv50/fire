#pragma once

namespace fire::sema {

struct ScopeContext;
struct FunctionContext;

struct NodeContext {
  ScopeContext* scope = nullptr;
  FunctionContext* func = nullptr;
};

} // namespace fire::sema