#include "a.h"
#include "b.h"
#include <funcb>

#define ENABLE_FOO 1

#if ENABLE_FOO
#define foo(x) FOO((x))
#endif

static int foo(int x) {
  return x + 1;
}
