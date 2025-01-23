#include "Lexer.h"
#include "Parser.h"
#include "Sema/Sema.h"
#include "Evaluator.h"

#include "Driver/Error.h"
#include "Driver/Driver.h"

#include "TypeInfo.h"
#include "Token/Token.h"
#include "Node/Node.h"

#include "Debug/Debug.h"

namespace fire::Debug {

using std::cout;
using std::endl;

void Test() {

  auto node = make_nd_root(
      {make_nd_func("func", {{"a", make_nd_type("int")}}, make_nd_block({}))});

  cout << node2s(node) << endl;

  try {
    sema::Sema(node).check_all();
  }
  catch (Error const& e) {
    cout << "in test func(): Error: " << e.get_message() << endl;
  }

  alertmsg("--end of Debug::Test()--");

  // std::exit(0);
}

} // namespace fire::Debug