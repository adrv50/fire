#include "alert.h"
#include "Sema/Sema.h"
#include "Error.h"

#define foreach(_Name, _Content) for (auto&& _Name : _Content)

namespace fire::semantics_checker {

// Sema::SemaFunction::SemaFunction(ASTPtr<AST::Function> func)
//     : func(func) {

//   if (!func->is_templated) {
//     this->result_type = Sema::GetInstance()->evaltype(func->return_type);
//     this->is_type_deducted = true;
//   }
// }

Sema::ArgumentCheckResult Sema::check_function_call_parameters(ASTVector args,
                                                               bool isVariableArg,
                                                               TypeVec const& formal,
                                                               TypeVec const& actual,
                                                               bool ignore_mismatch) {

  if (actual.size() < formal.size()) // 少なすぎる
    return ArgumentCheckResult::TooFewArguments;

  // 可変長引数ではない
  // => 多すぎる場合エラー
  if (!isVariableArg && actual.size() > formal.size()) {
    return ArgumentCheckResult::TooManyArguments;
  }

  if (!ignore_mismatch) {
    int _i = 0;

    for (auto f = formal.begin(), a = actual.begin(); f != formal.end(); f++, a++, _i++) {
      if (!f->equals(*a)) {
        return {ArgumentCheckResult::TypeMismatch, _i};
      }
    }
  }

  return ArgumentCheckResult::Ok;
}

} // namespace fire::semantics_checker
