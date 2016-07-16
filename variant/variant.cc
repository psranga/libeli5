// ELI5 Variant. 
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
struct ES0 {};
struct ES1 {};
struct ES2 {};
struct ES3 {};
struct ES4 {};
struct ES5 {};
struct ES6 {};
struct ES7 {};
struct ES8 {};
struct ES9 {};
struct ES10 {};
struct ES11 {};
struct ES12 {};
struct ES13 {};
struct ES14 {};
struct ES15 {};

// Some templated structs and specialiations so that we can do:
// Variant<double, int> v{1.25}; and then
// TypeToIndex<Variant, double>::index and get 0 etc. This will be used
// in our getters to error out if we try to get a different type than
// we put in.
template <typename VariantType, typename T>
struct TypeToIndex {
  // Only specializations will be used.
};

template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType0> {
  static constexpr int index = 0;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType1> {
  static constexpr int index = 1;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType2> {
  static constexpr int index = 2;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType3> {
  static constexpr int index = 3;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType4> {
  static constexpr int index = 4;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType5> {
  static constexpr int index = 5;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType6> {
  static constexpr int index = 6;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType7> {
  static constexpr int index = 7;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType8> {
  static constexpr int index = 8;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType9> {
  static constexpr int index = 9;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType10> {
  static constexpr int index = 10;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType11> {
  static constexpr int index = 11;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType12> {
  static constexpr int index = 12;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType13> {
  static constexpr int index = 13;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType14> {
  static constexpr int index = 14;
};
template <typename VariantType>
struct TypeToIndex<VariantType, typename VariantType::ValueType15> {
  static constexpr int index = 15;
};

}  // namespace internal

// CoreClassDefinition:
template <typename T0 = internal::ES0, typename T1 = internal::ES1, typename T2 = internal::ES2, typename T3 = internal::ES3, typename T4 = internal::ES4, typename T5 = internal::ES5, typename T6 = internal::ES6, typename T7 = internal::ES7, typename T8 = internal::ES8, typename T9 = internal::ES9, typename T10 = internal::ES10, typename T11 = internal::ES11, typename T12 = internal::ES12, typename T13 = internal::ES13, typename T14 = internal::ES14, typename T15 = internal::ES15>
struct Variant {
  union {
    // Wrap in a struct so that using the same type twice will work.
    // Variant<int, int> x. May happen if code is autogenerated etc.
    // Remove?
    struct { T0 v; } s0;
    struct { T1 v; } s1;
    struct { T2 v; } s2;
    struct { T3 v; } s3;
    struct { T4 v; } s4;
    struct { T5 v; } s5;
    struct { T6 v; } s6;
    struct { T7 v; } s7;
    struct { T8 v; } s8;
    struct { T9 v; } s9;
    struct { T10 v; } s10;
    struct { T11 v; } s11;
    struct { T12 v; } s12;
    struct { T13 v; } s13;
    struct { T14 v; } s14;
    struct { T15 v; } s15;
  };

  // Keep field num second so that accidental usage of Variant as a native
  // union has a chance to succeed.
  int field_num = -1;

  // Constructors to initialize the union from each of member types.
  explicit Variant(const T0& v) : s0{v}, field_num(0) {}
  explicit Variant(const T1& v) : s1{v}, field_num(1) {}
  explicit Variant(const T2& v) : s2{v}, field_num(2) {}
  explicit Variant(const T3& v) : s3{v}, field_num(3) {}
  explicit Variant(const T4& v) : s4{v}, field_num(4) {}
  explicit Variant(const T5& v) : s5{v}, field_num(5) {}
  explicit Variant(const T6& v) : s6{v}, field_num(6) {}
  explicit Variant(const T7& v) : s7{v}, field_num(7) {}
  explicit Variant(const T8& v) : s8{v}, field_num(8) {}
  explicit Variant(const T9& v) : s9{v}, field_num(9) {}
  explicit Variant(const T10& v) : s10{v}, field_num(10) {}
  explicit Variant(const T11& v) : s11{v}, field_num(11) {}
  explicit Variant(const T12& v) : s12{v}, field_num(12) {}
  explicit Variant(const T13& v) : s13{v}, field_num(13) {}
  explicit Variant(const T14& v) : s14{v}, field_num(14) {}
  explicit Variant(const T15& v) : s15{v}, field_num(15) {}

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
  // Varint<double, int> v{1.25};
  // ...
  // double x{v};
  explicit operator T0&() {
    assert(field_num == 0);
    return s0.v;
  }
  explicit operator T1&() {
    assert(field_num == 1);
    return s1.v;
  }
  explicit operator T2&() {
    assert(field_num == 2);
    return s2.v;
  }
  explicit operator T3&() {
    assert(field_num == 3);
    return s3.v;
  }
  explicit operator T4&() {
    assert(field_num == 4);
    return s4.v;
  }
  explicit operator T5&() {
    assert(field_num == 5);
    return s5.v;
  }
  explicit operator T6&() {
    assert(field_num == 6);
    return s6.v;
  }
  explicit operator T7&() {
    assert(field_num == 7);
    return s7.v;
  }
  explicit operator T8&() {
    assert(field_num == 8);
    return s8.v;
  }
  explicit operator T9&() {
    assert(field_num == 9);
    return s9.v;
  }
  explicit operator T10&() {
    assert(field_num == 10);
    return s10.v;
  }
  explicit operator T11&() {
    assert(field_num == 11);
    return s11.v;
  }
  explicit operator T12&() {
    assert(field_num == 12);
    return s12.v;
  }
  explicit operator T13&() {
    assert(field_num == 13);
    return s13.v;
  }
  explicit operator T14&() {
    assert(field_num == 14);
    return s14.v;
  }
  explicit operator T15&() {
    assert(field_num == 15);
    return s15.v;
  }

  // const version of type conversion operator.
  explicit operator const T0&() const {
    assert(field_num == 0);
    return s0.v;
  }
  explicit operator const T1&() const {
    assert(field_num == 1);
    return s1.v;
  }
  explicit operator const T2&() const {
    assert(field_num == 2);
    return s2.v;
  }
  explicit operator const T3&() const {
    assert(field_num == 3);
    return s3.v;
  }
  explicit operator const T4&() const {
    assert(field_num == 4);
    return s4.v;
  }
  explicit operator const T5&() const {
    assert(field_num == 5);
    return s5.v;
  }
  explicit operator const T6&() const {
    assert(field_num == 6);
    return s6.v;
  }
  explicit operator const T7&() const {
    assert(field_num == 7);
    return s7.v;
  }
  explicit operator const T8&() const {
    assert(field_num == 8);
    return s8.v;
  }
  explicit operator const T9&() const {
    assert(field_num == 9);
    return s9.v;
  }
  explicit operator const T10&() const {
    assert(field_num == 10);
    return s10.v;
  }
  explicit operator const T11&() const {
    assert(field_num == 11);
    return s11.v;
  }
  explicit operator const T12&() const {
    assert(field_num == 12);
    return s12.v;
  }
  explicit operator const T13&() const {
    assert(field_num == 13);
    return s13.v;
  }
  explicit operator const T14&() const {
    assert(field_num == 14);
    return s14.v;
  }
  explicit operator const T15&() const {
    assert(field_num == 15);
    return s15.v;
  }

  // Applies a function to the stored value. If we have N types in the variant
  // we will need N functions to be passed in, so that we can pick the appropriate
  // one and invoke it. We require that the N functions be bundled up in a struct
  // or class, with name 'Run', and taking a single parameter which is the
  // type to which that function should be applied.
  template <typename Dispatcher>
  void DispatchUsing(Dispatcher& dispatcher) const {
    switch (field_num) {
      case 0:
        DispatchOneValue(s0.v, dispatcher);
        break;
      case 1:
        DispatchOneValue(s1.v, dispatcher);
        break;
      case 2:
        DispatchOneValue(s2.v, dispatcher);
        break;
      case 3:
        DispatchOneValue(s3.v, dispatcher);
        break;
      case 4:
        DispatchOneValue(s4.v, dispatcher);
        break;
      case 5:
        DispatchOneValue(s5.v, dispatcher);
        break;
      case 6:
        DispatchOneValue(s6.v, dispatcher);
        break;
      case 7:
        DispatchOneValue(s7.v, dispatcher);
        break;
      case 8:
        DispatchOneValue(s8.v, dispatcher);
        break;
      case 9:
        DispatchOneValue(s9.v, dispatcher);
        break;
      case 10:
        DispatchOneValue(s10.v, dispatcher);
        break;
      case 11:
        DispatchOneValue(s11.v, dispatcher);
        break;
      case 12:
        DispatchOneValue(s12.v, dispatcher);
        break;
      case 13:
        DispatchOneValue(s13.v, dispatcher);
        break;
      case 14:
        DispatchOneValue(s14.v, dispatcher);
        break;
      case 15:
        DispatchOneValue(s15.v, dispatcher);
        break;
    }
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
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES0& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES1& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES2& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES3& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES4& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES5& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES6& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES7& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES8& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES9& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES10& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES11& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES12& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES13& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES14& v, Dispatcher& dispatcher) const {
  }
  template <typename Dispatcher>
  void DispatchOneValue(const internal::ES15& v, Dispatcher& dispatcher) const {
  }

  // Make the template params available. Used by the indexed getter
  // utility below.
  typedef T0 ValueType0;
  typedef T1 ValueType1;
  typedef T2 ValueType2;
  typedef T3 ValueType3;
  typedef T4 ValueType4;
  typedef T5 ValueType5;
  typedef T6 ValueType6;
  typedef T7 ValueType7;
  typedef T8 ValueType8;
  typedef T9 ValueType9;
  typedef T10 ValueType10;
  typedef T11 ValueType11;
  typedef T12 ValueType12;
  typedef T13 ValueType13;
  typedef T14 ValueType14;
  typedef T15 ValueType15;
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
template <typename VariantType>
struct TypeHelper<0, VariantType> {
  typedef typename VariantType::ValueType0 value_type;
};
template <typename VariantType>
struct TypeHelper<1, VariantType> {
  typedef typename VariantType::ValueType1 value_type;
};
template <typename VariantType>
struct TypeHelper<2, VariantType> {
  typedef typename VariantType::ValueType2 value_type;
};
template <typename VariantType>
struct TypeHelper<3, VariantType> {
  typedef typename VariantType::ValueType3 value_type;
};
template <typename VariantType>
struct TypeHelper<4, VariantType> {
  typedef typename VariantType::ValueType4 value_type;
};
template <typename VariantType>
struct TypeHelper<5, VariantType> {
  typedef typename VariantType::ValueType5 value_type;
};
template <typename VariantType>
struct TypeHelper<6, VariantType> {
  typedef typename VariantType::ValueType6 value_type;
};
template <typename VariantType>
struct TypeHelper<7, VariantType> {
  typedef typename VariantType::ValueType7 value_type;
};
template <typename VariantType>
struct TypeHelper<8, VariantType> {
  typedef typename VariantType::ValueType8 value_type;
};
template <typename VariantType>
struct TypeHelper<9, VariantType> {
  typedef typename VariantType::ValueType9 value_type;
};
template <typename VariantType>
struct TypeHelper<10, VariantType> {
  typedef typename VariantType::ValueType10 value_type;
};
template <typename VariantType>
struct TypeHelper<11, VariantType> {
  typedef typename VariantType::ValueType11 value_type;
};
template <typename VariantType>
struct TypeHelper<12, VariantType> {
  typedef typename VariantType::ValueType12 value_type;
};
template <typename VariantType>
struct TypeHelper<13, VariantType> {
  typedef typename VariantType::ValueType13 value_type;
};
template <typename VariantType>
struct TypeHelper<14, VariantType> {
  typedef typename VariantType::ValueType14 value_type;
};
template <typename VariantType>
struct TypeHelper<15, VariantType> {
  typedef typename VariantType::ValueType15 value_type;
};

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
  void Run(const Si& si) {
    cout << "Dispatch Si" << endl;
  }
  void Run(const Sd& sd) {
    cout << "Dispatch Sd" << endl;
  }
};

struct DispatcherSi {
  void Run(const Si& si) {
    cout << "Dispatch Si" << endl;
  }
};

static DioTest Test_DispatchBoth = []() {
  eli5::Variant<Si, Sd> v{Sd{104.50}};
  DispatcherBoth dispatcher_both;
  v.DispatchUsing(dispatcher_both);
};

static DioTest Test_FallbackDispatch = []() {
  cout << endl << "Test_FallbackDispatch" << endl;
  eli5::Variant<Si> v{Si{106}};
  DispatcherSi dispatcher_si;
  v.DispatchUsing(dispatcher_si);
};

static DioTest Test_GetOrDie = []() {
  eli5::Variant<Si, Sd> v{Si{107}};
  Si out = eli5::GetOrDie<0>(v);
  cout << "Si/out: " << out.v << endl;
};

}

