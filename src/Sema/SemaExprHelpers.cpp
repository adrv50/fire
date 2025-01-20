#include "Driver/Error.h"
#include "Sema/Sema.h"

namespace fire::sema {

TypeInfo ExprEval::expect_enumerator_type(Node* node) {
  auto type = this->eval(node);

  if (!type.is_enumerator()) {
    Error(node,
          "expected enumerator type expression, but found '" + type.to_string() + "'")
        .crash();
  }

  return type;
}

} // namespace fire::sema