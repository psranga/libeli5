#include "a.h"
#include "b.h"
#include <funcb>

using a::funca;
using std::funcb;

struct A : public {
  int a;
  int b;
  void foo(int x) {
    b = 200;
  }

  void bar();

  void bar2() {
    funca();
    funcb();
  }
};

void A::bar() {
  funca();
}

int factorial(int n) {
  return factorial(n-1);
}

vec3 ComputeSomething(const A::Point& point, double t) {
}

namespace {

void foo_bar(int x);

}

void foo_bar2(int x) {
  cout << "ok";
}

typedef unsigned int Word;

template <typename T>
struct S {
  vector<T> v;

  S() : v(100) {}
};

template <typename T>
int is_done(T x) {
  return x < 0;
}

template <typename T>
class C {
  vector<T> v;

  C() : v(100) {}
};

template <class T>
int is_donec(T x) {
  return x < 0;
}

template <class T>
class C2 {
  vector<T> v;

  C2() : v(100) {}
};

template <class T>
struct S2 {
  vector<T> v;

  S2() : v(100) {}
};

struct EmptyStruct0 {};
namespace foo {

void bar(int x) {
  return x + 1;
}

}  // namespace foo
namespace foo {
namespace internal {

void bar(int x) {
  return x + 1;
}

}  // namespace internal
}  // namespace foo

namespace internal {

using eli5::Get;

struct Foo3 {
  void foo3() {
    Get(100);
  }
}

}

namespace internal {

struct Foo4 {
  void foo4() {
    Get(100);
  }
}

}

inline void influnc(int x) {
  return x*x;
}

template <>
int foofunc<int>(int x) {
  return x/2;
}

// Struct template specialization that fits on a single line.
template <typename T>
struct TS<0, T> {
  typedef T::value_type value_type;
};

// Class template specialization that fits on a single line.
template <typename T>
class TS<0, T> {
  typedef T::value_type value_type;
};
