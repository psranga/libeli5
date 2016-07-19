// ELI5 Variant. {{!This is a Mustache template file from which variant.h is generated.
// You can read this file to understand the implementation, but this file is not
// valid C++. Use the program 'genvariant.py' in this directory to generate variant.h}}
namespace eli5 {

// Variant: a statically-checked  type-safe union. Contains one of multiple
// types. Setting a type and attempting to read it out as another results in a
// compile-time error.
//
// The basic idea behind Eli5's Variant class is to create a thin
// wrapper around a C++ union. We add a single numeric field to
// track which of the union's members is set.
//
// We keep the implementation simple by allowing at most 16 values to
// multiplexed in a variant. The basic idea is to create a variant that
// always contains 16 values. For the types not set by the user, we default
// to empty structs.
//
// The operations provided are: construct, get, check, and dispatch.
//
// Example:
//
//  Variant<int, double> v{1.25};  // Can hold either an int or a double.
//
//  double zd = v.GetOrDie<double>();  // Typed get method.
//  int zi = v.GetOrDie<int>();        // Fails with assert at runtime.
//
//  // Check what type is in the variant before accessing.
//  bool has_double = v.Is<double>(); // True.
//  bool has_int = v.Is<int>(); // False.
//
//  // Dispatching.
//  struct Dispatcher {
//    void Run(int x) {
//      cout << "got int: " << x << endl;
//    }
//
//    void Run(double y) {
//      cout << "got double: " << y << endl;
//    }
//  };
//
//  // prints 'got double: 1.25'
//  v.DispatchUsing(Dispatcher());
//
//  // "Indexed" get. I think typed get is more readable.
//  double wd = eli5::GetOrDie<1>(v);
//  int wi = eli5::GetOrDie<0>(v);     // Fails with assert at runtime.
//
//  TODO: create Indexed versions of Is.
//
//  // Can also get by simply casting to destination type.
//  // This feature may be removed in the future, in the interest of
//  // economy of interface.
//  double x(v);  // Or get simply by converting to the type
//  int y(x);     // This will fail at runtime with an assert.
//                //   because type conversions are marked 'explicit'.
//
//  double z(static_cast<double>(v));  // what happens under the hood.
//
// In 'dispatch', we accept a bunch of functions that accept a single argument
// of each of the types in the variant. The Variant will then invoke the
// function corresponding to the type within it, passing the value as an
// argument. The functions are "bunched together" as multiple overloaded functions
// named 'Run' in a struct. The struct is passed as a parameter to
// the variant. See example below.
//
// Construction is the only way to set a value. You can't set a different
// type after construction. This is more of a design decision,
// rather than a limitaiton of C++. This may change in the future.
//
// Suggested reading order (search for these words using your editor):
//   Introduction (you just finished this section)
//   CoreClassDefinition (read through the entire class, esp. DispatchUsing)
//   TypeToIndex (to understand how checking works)
//   IndexedGet
namespace internal {

// A set of dummy empty types for variant template types that are not set.
{{#repeat}}
struct ES{{i}} {
  bool operator==(const ES{{i}}& other) const {
    return true;
  }
};
{{/repeat}}

// Some templated structs and specialiations so that we can do:
// Variant<double, int> v{1.25}; and then
// TypeToIndex<Variant, double>::index and get 0 etc. This will be used
// in our getters to error out if we try to get a different type than
// we put in.
template <typename VariantType, typename T>
struct TypeToIndex {
  // Only specializations will be used.
};

{{#repeat}}
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType{{i}}> {
  static constexpr int index = {{i}};
};
{{/repeat}}

}  // namespace internal

// CoreClassDefinition:
template <{{#repeat_and_comma_contact}}typename T{{i}} = internal::ES{{i}}{{/repeat_and_comma_contact}}>
struct Variant {
  // The union that multiplexes the values.
  union U {
    // Wrap in a struct so that using the same type twice will work.
    // E.g., Variant<int, int> x.
    {{#repeat}}
    struct { T{{i}} v; } s{{i}};
    {{/repeat}}
    U() {};
    ~U() {};
    U(const U& other) {}
  };

  U storage;

  // Keep field num second so that accidental usage of Variant as a native
  // union has a chance to succeed.
  int field_num = -1;

  // Constructors to initialize the union from each of member types.
  {{#repeat}}
  Variant(const T{{i}}& v) {
    field_num = {{i}};
    // Placement new more precise than assignment in the general case.
    // We're doing that instead of: storage.si = v.
    new (&storage.s{{i}}.v) T{{i}}(v);
  }
  {{/repeat}}

  // Workaround for C's original sin: no native string type. In C++, values
  // will go through at most one type conversion. So something like this
  // convenient notation won't work:
  //   vector<variant<string>> v{"abcd"};
  // Because "abcd" is of type 'const char*' and it has go through two
  // conversions ('const char *' -> string -> Variant) to become a Variant.
  // Providing this delegating constructor enables the notation above to
  // work, by allowing 'const char *' to become a Variant in a single step.
  // Adding this constructor will still result in a compile time error if
  // a string literal tries to get into a Variant for which 'string' isn't
  // one of the types. See test Test_ListInitVector for an example.
  Variant(const char* ca) : Variant(string(ca)) {};

  ~Variant() {
    // Call destructor of stored value.
    switch (field_num) {
      {{#repeat}}
      case {{i}}:
        // Directly call the destructor.
        storage.s{{i}}.v.~T{{i}}();
        break;
      {{/repeat}}
    }
  }

  Variant(const Variant& other) {
    field_num = other.field_num;
    switch (field_num) {
      {{#repeat}}
      case {{i}}:
        // Placement new more precise than assignment in the general case.
        // We're doing that instead of: storage.si = other.storage.si.
        new (&storage.s{{i}}.v) T{{i}}(other.storage.s{{i}}.v);
        break;
      {{/repeat}}
    }
  }

  // Typed getter function. Allows us to do:
  // Variant<int, double> v{1.50};
  // ...
  // double x = v.get<double>();
  //
  // Syntax sugar around type conversion operator.
  template <typename T>
  T& GetOrDie() {
    assert((internal::TypeToIndex<Variant, T>::index) == field_num);
    return static_cast<T&>(*this);
  }

  // Check function. Check if variant holds a certain type.
  // Variant<int, double> v{1.50};
  // ...
  // bool has_double = v.Is<double>();  // true
  // bool has_int = v.Is<int>();  // false
  template <typename T>
  bool Is() {
    return ((internal::TypeToIndex<Variant, T>::index) == field_num);
  }

  // Typed getter that returns a mutable reference.
  // A non-const refernce to member, for direct poking.
  template <typename T>
  T& Mutable() const {
    assert((internal::TypeToIndex<Variant, T>::index) == field_num);
    return static_cast<T&>(*this);
  }

  // Type conversion operators. Allows concise code like:
  // Variant<double, int> v{1.25};
  // ...
  // double x{v};
  {{#repeat}}
  explicit operator T{{i}}&() {
    assert(field_num == {{i}});
    return storage.s{{i}}.v;
  }
  {{/repeat}}

  // const version of type conversion operator.
  {{#repeat}}
  explicit operator const T{{i}}&() const {
    assert(field_num == {{i}});
    return storage.s{{i}}.v;
  }
  {{/repeat}}

  // Applies a function to the stored value. If we have N types in the variant
  // we will need N functions to be passed in, so that we can pick the appropriate
  // one and invoke it. We require that the N functions be bundled up in a struct
  // or class, with name 'Run', and taking a single parameter which is the
  // type to which that function should be applied.
  template <typename Dispatcher>
  void DispatchUsing(Dispatcher& dispatcher) const {
    switch (field_num) {
      {{#repeat}}
      case {{i}}:
        DispatchOneValue(storage.s{{i}}.v, dispatcher);
        break;
      {{/repeat}}
    }
  }

  // Check if two variants are equal. Iff the field types are same and values
  // are equal by operator==.
  bool operator==(const Variant& other) const {
    if (field_num != other.field_num) {
      return false;
    }
    switch (field_num) {
      {{#repeat}}
      case {{i}}:
        return storage.s{{i}}.v == other.storage.s{{i}}.v;
        break;
      {{/repeat}}
    }
    return false;
  }

  // Inequality. Literally negation of equality.
  bool operator!=(const Variant& other) const {
    return !(*this == other);
  }

  // Dispatch function that will call the dispatch method within the
  // user's dispatcher. We need this indirection so that we get a chance
  // make the default dispatchers participate in overload resolution.
  // If the user makes a two-element variant and the supplied dispatcher
  // supplies functions for those two types, we want the default dispatching
  // functions to be used for the switch cases 3 through max.
  template <typename T, typename Dispatcher>
  void DispatchOneValue(const T& v, Dispatcher& dispatcher) const {
    dispatcher.Run(v);
  }

  // Overloads that will match for the template params that are set to
  // defaults. i.e., when user create a variant with fewer than max
  // elements.
  {{#repeat}}
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES{{i}}& v, Dispatcher& dispatcher) const {
  }
  {{/repeat}}

  // Make the template params available. Used by the indexed getter
  // utility below.
  {{#repeat}}
  typedef T{{i}} ValueType{{i}};
  {{/repeat}}
};

namespace internal {

// IndexedGet
// Indexed getter for those that prefer this style as opposed to the typed getter
// member function. Allows us to do:
//
// Variant<int, double> v{1.25};
// ...
// double x = eli::GetOrDie<1>(v);
//
// Works by defining a templated function named GetOrDie that is a thin wrapper around
// the type conversion operator defined above. The key trick is in going from a
// number like 1 to the type 'double'. We use a set of structs named 'TypeHelper'
// to do this. We do this by templating it on an int N. We specialize the template
// for different values of N. We define a typedef named 'value_type' in *all*
// specializations, but which points to variant's 0th type in the 0th specialization,
// the variants 1st type in the 1st specialization and so on.

// TypeHelper is set of structs so that we can do TypeHelper<0,
// Variant>::value_type and get 0th type of the variant etc. We need this so
// that we can express all the return values of the indexed getter function
// using the same literal program text.
//
// These structs are the reason why we made the templates types available as
// typedefs named ValueType0 etc in the Variant class above.
template <int N, typename VariantType>
struct TypeHelper {
  // This should never be used. Only the specializations below will be used.
};

// Specialization of TypeHelper that creates a typedef for the 0th type of the 
// variant.
{{#repeat}}
template <typename VariantType>
struct TypeHelper<{{i}}, VariantType> {
  typedef typename VariantType::ValueType{{i}} value_type;
};
{{/repeat}}

}  // namespace internal

// The indexed getter.
template <int N, typename VariantType>
typename internal::TypeHelper<N, VariantType>::value_type& GetOrDie(VariantType& v) {
  return static_cast<
      typename internal::TypeHelper<N, VariantType>::value_type&>(v);
}

}  // namespace eli5

// Tests. Written using the Diogenes "framework".
namespace {

static DioTest Test_Example = []() {
  eli5::Variant<int, double> v{1.25};

  double zd = v.GetOrDie<double>();  // Typed get method.
  cout << "zd: " << zd << endl;
  cout << "Is(int): " << v.Is<int>() << endl;
  cout << "Is(double): " << v.Is<double>() << endl;

  // int zi = v.GetOrDie<int>();        // Fails with assert at runtime.

  double wd = eli5::GetOrDie<1>(v);  // 'Indexed' get.
  cout << "wd: " << wd << endl;

  //int wi = eli5::GetOrDie<0>(v);     // Fails with assert at runtime.

  double x(v);  // Or get simply by converting to the type
  cout << "x: " << x << endl;

  // int y(v);     // Fails with assert at runtime.
  // cout << "y: " << y << endl;

  double z(static_cast<double>(v));  // what happens under the hood.
  cout << "z: " << z << endl;
};

struct Sd {
  double v = 100.25;
  Sd() {};
  Sd(double x) : v(x) {};
};

struct Si {
  int v = 102;
  Si() {};
  Si(int x) : v(x) {};
};

static DioTest Test_Basic = []() {
  eli5::Variant<int> v{100};

  cout << "sizeof: " << sizeof(v) << endl;
  int x(v);
  cout << "dx: " << x << endl;
  DioExpect(x == 100);
};

static DioTest Test_TwoTypes = []() {
  eli5::Variant<Si, Sd> v{Si{100}};
  Si x(v);
  DioExpect(x.v == 100);
};

static DioTest Test_TwoTypes2 = []() {
  eli5::Variant<Si, Sd> v{Sd{103.50}};
  Sd x(v);
  DioExpect(x.v == 103.50);
};

struct DispatcherBoth {
  int flag = -1;
  void Run(const Si& si) {
    cout << "Dispatch Si" << endl;
    flag = 0;
  }
  void Run(const Sd& sd) {
    cout << "Dispatch Sd" << endl;
    flag = 1;
  }
};

struct DispatcherSi {
  int flag = -1;
  void Run(const Si& si) {
    cout << "Dispatch Si" << endl;
    flag = 0;
  }
};

static DioTest Test_DispatchBoth = []() {
  eli5::Variant<Si, Sd> v{Sd{104.50}};
  DispatcherBoth dispatcher_both;
  v.DispatchUsing(dispatcher_both);
  DioExpect(dispatcher_both.flag == 1);
};

static DioTest Test_FallbackDispatch = []() {
  cout << endl << "Test_FallbackDispatch" << endl;
  eli5::Variant<Si> v{Si{106}};
  DispatcherSi dispatcher_si;
  v.DispatchUsing(dispatcher_si);
  DioExpect(dispatcher_si.flag == 0);
};

static DioTest Test_GetOrDie = []() {
  eli5::Variant<Si, Sd> v{Si{107}};
  Si out = eli5::GetOrDie<0>(v);
  cout << "Si/out: " << out.v << endl;
  DioExpect(out.v == 107);
};

static DioTest Test_Copy = []() {
  typedef eli5::Variant<int, string> V;
  V v("abcd");
  V w(v);
  DioExpect(v.GetOrDie<string>() == w.GetOrDie<string>());
};

static DioTest Test_Equality = []() {
  typedef eli5::Variant<int, string> V;
  V v("abcd");
  V w("abcd");
  V z(1);
  DioExpect(v == w);
  DioExpect(v != z);
};

static DioTest Test_ListInit = []() {
  eli5::Variant<int, string> v{"abcd"};
  string out = eli5::GetOrDie<1>(v);
  DioExpect(out == "abcd");
};

// Check that we can store Variant in a vector.
static DioTest Test_Vector = []() {
  typedef eli5::Variant<int, string> V;
  vector<V> vs{V{"abcd"}, V{1}};
  DioExpect(vs.size() == 2);
  DioExpect(vs[0].GetOrDie<string>() == "abcd");
  DioExpect(vs[1].GetOrDie<int>() == 1);
};

// Check that can initialize a Variant with vector list initializaton syntax.
static DioTest Test_ListInitVector = []() {
  typedef eli5::Variant<int, string> V;
  vector<V> vs{1, "abcd", 2};
  DioExpect(vs.size() == 3);
  DioExpect(vs[0].GetOrDie<int>() == 1);
  DioExpect(vs[1].GetOrDie<string>() == "abcd");
  DioExpect(vs[2].GetOrDie<int>() == 2);
};

// Check that compile time error if trying to add a string literal into
// a Variant that lacks string. TODO: figure out how to do must-not-compile
// checks. Diogenes is probably the wrong place for such tests.
static DioTest Test_ListInitVector_NoStringInVariant = []() {
  //typedef eli5::Variant<int, double> V;
  //vector<V> vs{1, 2.25, "abcd", 3};
  //cout << "out5: " << vs.size() << endl;
};

}
