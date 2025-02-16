#pragma once

// #include "alert.h"

#include <cassert>
#include "node2s.h"

#if _FIRE_DEBUG_

  #include "Color.h"
  #include <sstream>

  #define debug if (1)

  #define alert                                                                          \
    fire::Debug::_alert(__FILE__, __LINE__, __PRETTY_FUNCTION__,                         \
                        (Color::Magenta + "alert").c_str())

  #define alertfmt(fmt, args...)                                                         \
    fire::Debug::_alert(__FILE__, __LINE__, __PRETTY_FUNCTION__,                         \
                        (Color::Magenta + "alertfmt %s", args).c_str())

  #define alertmsg(coutmsg)                                                              \
    fire::Debug::_alert(__FILE__, __LINE__, __PRETTY_FUNCTION__,                         \
                        (Color::Magenta + "alertmsg " + [&]() -> string {                \
                          std::stringstream ss;                                          \
                          ss << Color::White << Color::Bold << coutmsg;                  \
                          return ss.str();                                               \
                        }())                                                             \
                            .c_str())

  #define alertexpr(expr) alertmsg("\"" #expr "\" = " << (expr))

  #define panic (alertmsg("panic!"), std::exit(1))

  #define todo_impl (alertmsg("not implemented"), std::exit(1))

#else

  #define debug if (0)

  #define alert ;
  #define alertfmt ;
  #define alertmsg ;
  #define alertexpr ;

  #define todo_impl                                                                      \
    (fprintf("\t%s:%zu: not implemented", __FILE__, __LINE__), std::exit(1))

#endif

namespace fire::Debug {

void _alert(char const* file, size_t line, char const* func, char const* fmt, ...);

Node* make_nd_root(Vec<Node*>&& nodes);

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args, Node* body);

Node* make_nd_func(char const* name, Vec<pair<char const*, Node*>>&& args,
                   Node* return_type, Node* body);

Node* make_nd_if(Node* cond, Node* then, Node* Else = nullptr);
Node* make_nd_let(char const* name, Node* type, Node* init);
Node* make_nd_block(Vec<Node*>&& nodes);

Node* make_nd_type(char const* name);
Node* make_nd_type(char const* name, Vec<Node*>&& tp_args, bool Mut = false,
                   bool Ref = false);
Node* make_nd_type(Vec<char const*>&& scope_resol, Vec<Node*>&& tp_args, bool Mut = false,
                   bool Ref = false);

Node* make_nd_expr(NodeKind kind, Node* lhs, Node* rhs);
Node* make_nd_scope_resol(Vec<char const*>&& scope_resol);
Node* make_nd_id(char const* name);
Node* make_nd_val(Object* obj);

//
void Test();

} // namespace fire::Debug