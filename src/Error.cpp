#include "Error.h"

namespace metro {

Error::Error(Token tok) {
}

Error::Error(AST::ASTPointer ast) {
}

Error& Error::set_message(std::string msg) {
}

Error& Error::emit() {
}

Error& Error::emit_as_hint() {
}

[[noreturn]]
void Error::stop() {
}

} // namespace metro