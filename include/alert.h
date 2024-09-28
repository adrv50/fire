#pragma once

#include <string>

#define COL_DEFAULT "\033[0m"
#define COL_BOLD "\033[1m"
#define COL_UNDERLINE "\033[4m"
#define COL_UNBOLD "\033[2m"

#define COL_BLACK "\033[30m"
#define COL_RED "\033[31m"
#define COL_GREEN "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_BLUE "\033[34m"
#define COL_MAGENTA "\033[35m"
#define COL_CYAN "\033[36;5m"
#define COL_GRAY "\e[38;5;235m"
#define COL_WHITE "\033[37m"

#define COL_BK_BLACK "\033[40m"
#define COL_BK_RED "\033[41m"
#define COL_BK_GREEN "\033[42m"
#define COL_BK_YELLOW "\033[43m"
#define COL_BK_BLUE "\033[44m"
#define COL_BK_MAGENTA "\033[45m"
#define COL_BK_CYAN "\033[46;5m"
#define COL_BK_WHITE "\033[47m"

#ifdef _METRO_DEBUG_

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <sstream>

#define debug(...) __VA_ARGS__;
#define alert printf("\t%s:%u\talert\n", strrchr(__FILE__, '/') + 1, __LINE__);

#define alertfmt(fmt, e...)                                                    \
  printf("\t%s:%u\talertfmt " COL_BOLD COL_WHITE fmt "\n" COL_DEFAULT,         \
         strrchr(__FILE__, '/') + 1, __LINE__, e)

#define alertmsg(e...)                                                         \
  ({                                                                           \
    std::stringstream ss;                                                      \
    ss << e;                                                                   \
    alertfmt("%s", ss.str().c_str());                                          \
  })

#define alertexpr(expr) alertmsg("\"" #expr "\" = " << (expr))

#define todo_impl (alertmsg("not implemented"), exit(1))
#define panic (alertmsg("panic !"), exit(1))

#define not_implemented(msg)                                                   \
  ({                                                                           \
    alertmsg(msg);                                                             \
    exit(1);                                                                   \
  })

#else
#define debug(...) ;
#define alert (void)0
#define alertfmt(...) (void)0
#define alertmsg(...) (void)0
#define alertexpr(...) (void)0
#define not_implemented(...) (void)0

#define todo_impl                                                              \
  {                                                                            \
    fprintf(stderr, "%s:%u: not implemented error\n", __FILE__, __LINE__);     \
    exit(1);                                                                   \
  }
#define panic                                                                  \
  {                                                                            \
    fprintf(stderr, "%s:%u: panic.\n", __FILE__, __LINE__);                    \
    exit(1);                                                                   \
  }
#endif