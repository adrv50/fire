#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define COL_DEFAULT "\033[0m"
#define COL_BOLD "\033[1m"

#define COL_BLACK "\033[30m"
#define COL_RED "\033[31m"
#define COL_GREEN "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_BLUE "\033[34m"
#define COL_MAGENTA "\033[35m"
#define COL_CYAN "\033[36;5m"
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

#define debug(...) __VA_ARGS__;
#define alert                                                        \
  printf("\t%s:%u\talert\n", strrchr(__FILE__, '/') + 1, __LINE__);
#define alertmsg(msg)                                                \
  printf("\t%s:%u\talertmsg " COL_BOLD COL_WHITE #msg                \
         "\n" COL_DEFAULT,                                           \
         strrchr(__FILE__, '/') + 1, __LINE__)
#define alertfmt(fmt, e...)                                          \
  printf("\t%s:%u\talertfmt " COL_BOLD COL_WHITE fmt                 \
         "\n" COL_DEFAULT,                                           \
         strrchr(__FILE__, '/') + 1, __LINE__, e)

#define todo_impl (alertmsg(not implemented), exit(1))
#define panic (alertmsg(panic !), exit(1))

#else
#define debug(...) ;
#define alert (void)0
#define alertmsg(...) (void)0
#define alertfmt(...) (void)0

#define todo_impl                                                    \
  {                                                                  \
    fprintf(stderr, "%s:%u: not implemented error\n", __FILE__,      \
            __LINE__);                                               \
    exit(1);                                                         \
  }
#define panic                                                        \
  {                                                                  \
    fprintf(stderr, "%s:%u: panic.\n", __FILE__, __LINE__);          \
    exit(1);                                                         \
  }
#endif