#include "a.h"
#include "b.h"
#include <funcb>

using std::pair;

struct S {
  void S_mymakepair(int x, int y) {
    return pair{x, y};
  }
};

void mymakepair(int x, int y) {
  return pair{x, y};
}

static int foo(int x) {
  return x + 1;
}
