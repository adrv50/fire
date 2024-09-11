#include "Parser/AST/AST.h"
#include "GC/GC.h"
#include "Interpreter/Interp.h"

namespace metro::gc {

using Memory = std::vector<Object*>;

static Memory heap;
static Memory lifemem;
static Memory temp;

static Memory* curmem;

void InitGC() {
  curmem = &heap;
}

void ExitGC() {
}

void ForceCollection() {
}

} // namespace metro::gc
