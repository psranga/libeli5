#ifndef _DIOGENES_H_
#define _DIOGENES_H_

// Diogenes: World's Simplest Unit Testing "Framework"?
// Purplerails <ranga@purplerails.com>
//
// See README.md in this directory for complete docs.
//
// A complete example follows. Save it as example.cpp and compile it:
//   g++ -std=c++11 -Wall example.cpp
//
// // ----- Start of example.cpp ------------
// #include "diogenes.h"
// #include <iostream>
//
// using std::cout;
// using std::endl;
//
// // The function being tested.
// static void ClearValue(int& i) {
//   int localvar = i + 2;
//
//   DioSnapshot(localvar, "immediate");
//   DioSnapshotOnExit(localvar, "on_exit");
//
//   i = 0;
//
//   localvar += 2;
// }
//
// // Testing the function above:
// // Creata lambda containing your test and assign it to
// // a variable of type DioTest.
// static DioTest Test_ClearInt = []() {
//   // Set up inputs.
//   int i = 12;
//
//   // Call the function being tested.
//   ClearValue(i);
//
//   // Check that the function worked.
//   DioExpect(i == 0);
//
//   // Also check the internal state that was snapshotted.
//   DioExpect(DioGetSnapshottedValue<int>("immediate") == 14);
//
//   // Note the lack of 'int' by passing an unused parameter 'i'. This
//   // style should probably be used sparingly.
//   DioExpect(DioGetSnapshottedValue("on_exit", i) == 16);
//
//   cout << "immediate: " <<
//     DioGetSnapshottedValue<int>("immediate") << endl;
//   cout << "on exit: " <<
//     DioGetSnapshottedValue<int>("on_exit") << endl;
// };
//
// // Register with filename, line number, and test name for
// // running a subset of tests.
// DIOTEST(Test_ClearInt2) = []() {
//   // Set up inputs.
//   int i = 12;
//
//   // Call the function being tested.
//   ClearValue(i);
//
//   // Check that the function worked.
//   DioExpect(i == 0);
// };
//
// int main() {
//   // Call this somewhere to run all tests in all files.
//   Diogenes::RunAll();
// }
// // ----- End of example.cpp ------------

#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

// A class representing a single test. A static member variable
// in this class is used to keep track of all tests created.
//
// Any void function can be a test. The test should call
// DioExpect within it to indicate success and failure.
class DioTest {
 public:
  // A test is any void function. The test should call DioExpect within it
  // to indicate success failure.
  typedef std::function<void()> Test;

  // A single test.
  Test t;
  const char* filename = nullptr;
  int linenum = -1;
  const char* test_name = nullptr;

  // Makes a test out of any void lambda, and adds it to all tests.
  // This is the core idea of Diogenes.
  template <class Lambda>
  DioTest(Lambda l)
      : t(l) {
    AllTests().push_back(this);
  }

  // Makes test with filename, line number and name of test also specified.
  // The macro DIOTEST will probably be more convenient.
  template <class Lambda>
  DioTest(const char *_filename, int _linenum, const char *_test_name, Lambda l)
      : t(l), filename(_filename), linenum(_linenum), test_name(_test_name) {
    AllTests().push_back(this);
  }

  // Create shell of a test with only filename, line number, and name of test set.
  // For use with DIOTEST macro below.
  DioTest(const char *_filename, int _linenum, const char *_test_name)
      : filename(_filename), linenum(_linenum), test_name(_test_name) {
    AllTests().push_back(this);
  }

  // Runs this test. Test passes if none of the calls it makes to DioExpect
  // indicate failure. If it makes no calls, that means the test passes.
  virtual void Run() const { t(); }

  // Called before test runs.
  virtual void Setup() {};

  // Called after test runs.
  virtual void Teardown() {};

  // Keeps track of all tests added so far. Workaround to keep this
  // header-only: put the static variable inside a function. If tests is a
  // class static var, we'd have to define it in a cpp file and link that in.
  static std::vector<DioTest*>& AllTests() {
    static std::vector<DioTest*> tests;
    return tests;
  }

  // run_spec is like: "test_name1,test_name2,filename.cc,..."
  static bool ShouldRunTest(DioTest* t, const string& run_spec) {
    if ((t->filename == nullptr) || (t->test_name == nullptr)) {
      return false;
    }
    return (run_spec.find(t->filename) != string::npos) ||
           (run_spec.find(t->test_name) != string::npos);
  }

  // Runs all tests. If run_spec is not null, runs the subset specified by that
  // spec.
  static void RunAll(const std::string& run_spec = "") {
    for (DioTest* f : AllTests()) {
      if (run_spec.empty() || ShouldRunTest(f, run_spec)) {
        f->Setup();
        RecordExpectStatusOrPrintResults(false /* ignored */,
                                         2 /* increment number of test run */);
        f->Run();
        f->Teardown();
      }
    }
    RecordExpectStatusOrPrintResults(false /* ignored */,
                                     0 /* print status */);
  }

  // If 'record' is true, keeps track of the number of failing and
  // passing tests. If 'record' is false, print a report to
  // stdout saying how many passed and failed.
  static void RecordExpectStatusOrPrintResults(bool value, int op) {
    static int num_tests_run = 0;
    static int passed = 0;
    static int failed = 0;
    if (op == 1) {
      if (value) {
        ++passed;
      } else {
        ++failed;
      }
    } else if (op == 2) {
      ++num_tests_run;
    } else if (op == 0) {
      std::cout << "Diogenes results: Ran " << num_tests_run << " tests. " << passed << "/"
                << (passed + failed) << " expects passed (" << failed
                << " failed)." << std::endl;
      assert(failed == 0);
    }
  }

  // Prefer the macro DioExpect below.
  static void DioExpect2(const std::string& expression_str, bool value) {
    RecordExpectStatusOrPrintResults(value, 1 /* record */);
    if (!value) {
      std::cerr << "Failed test: '" << expression_str << "'" << std::endl;
    }
  }
};

// I'm not happy about this macro, since it's not very ELI5. But running
// a subset of tests is an important capability. So I'm reluctantly
// adding this macro so support that feature.
//
// Macro to register a test with filename, line number, and name info.
// Usage:
//
//   DIOTEST(Test_Foo) = []() {
//   };
//
// Almost same syntax as the unnamed test syntax: you assign a lambda to
// something. Under the hood, a unique class named after the test gets
// created. A *static* variable of type lambda is declared within the class.
// Finally, the first part of the code to set the static variable is emitted.
//
// Expands to something like:
//
// struct Test_Foo_Class : public DioTest {
//   static DioTest::Test l;
//   Test_Foo_Class(filename, linenum, name) : DioTest(filename, linenum, name) {};
// };
// Test_Foo_Class::l /* your lambda definition gets assigned to this variable */
#define DIOTEST(test_name) \
struct test_name##_Class : public DioTest { \
  static DioTest::Test l; \
  void Run() const override { \
    l(); \
  } \
  test_name##_Class(const char *filename, int linenum, const char *test_name) \
      : DioTest(filename, linenum, test_name){}; \
}; \
\
static test_name##_Class test_name(__FILE__, __LINE__, #test_name); \
DioTest::Test test_name##_Class::l \

// Macro to textually mark namespaces that are used only for testing and that
// can be removed if building a binary that lacks debug code. cpp-makeheader
// uses this to avoid writing declaraions for functions etc in namespaces
// declared using this macro.
//
// Needed because some parts of the standard library require functions defined
// in std namespace. E.g., operator<< for custom types.
#define DIONAMESPACE namespace

// Syntax sugar: Allow Diogenes::RunAll() to run all tests.
typedef DioTest Diogenes;

// Prefer this if you are lazy. If EXP evaluates to true, then the
// test passes.
#define DioExpect(EXP) DioTest::DioExpect2((#EXP), (EXP))

// ================= Observability: snapshotting variables =============
// The basic idea is to have one cache per C++ type in which snapshotted
// values are stored. Snapshotted values are keyed by a string key for
// later retrieval. Cache singleton-ness is achieved by making it a static
// variable within this function.`
//
// This function gets or sets from the cache, depending upon the
// arguments. This is a simple way to ensure that get and set operations
// both use the same cache.
//
// Snapshots are created by copying, so the things being snapshotted
// should be copyable.
//
// op : 0 means get, 1 means set.
//
// key: the string using which this snapshotted value can be retrieved.
//
// val: the value to be inserted into the cache. Ignored during get.
//      Cannot be null during set. This argument is a pointer to allow setting
//      it to null in get.
//
// ok: During get, if not null, set to found/not status.
//     During set, if not null, will be set to true.
//
// Return value:
//
// Return value is a pointer to allow returning null during set.
//
// During get: Returns a const pointer to cached value, if found. Or nullptr
//             if not found.
//
// During set: Returns nullptr always.
//
template <class T>
const T* DioGetOrSetSnapshot(int op, const std::string& key,
    const T* val, bool* ok) {
  static std::unordered_map<std::string, T> cache;

  if (op == 0) {
    const auto& loc = cache.find(key);
    if (loc != cache.end()) {
      if (ok != nullptr) {
        *ok = true;
      }
      return &(loc->second);
    }
    if (ok != nullptr) {
      *ok = false;
    }
    return nullptr;
  } else if (op == 1) {
    cache.insert({key, *val});
    if (ok != nullptr) {
      *ok = true;
    }
    return nullptr;
  } else {
    if (ok != nullptr) {
      *ok = false;
    }
    return nullptr;
  }
}

// Thin wrapper around DioGetOrSetSnapshot that adapts it into a
// getter that returns a const reference to cached value.
//
// TODO: Crash if value wasn't found.
template <class T>
const T& DioGetSnapshottedValue(const std::string& key) {
  bool found = false;
  const T* output = DioGetOrSetSnapshot(0 /* get */, key,
      static_cast<const T*>(nullptr), &found);
  assert(found == true);
  return *output;
}

// Syntax sugar. Passing an extra, unused argument can force
// template argument type deduction. Could be more readable in
// some cases then specifying the type explicitly.
//
// TODO: Crash if value wasn't found.
template <class T>
const T& DioGetSnapshottedValue(const std::string& key,
    const T& unused) {
  return DioGetSnapshottedValue<T>(key);
}

// Snapshot a variable on exit from scope. Pretty simple: simply arranges
// for snapshotting to happen in the destructor. Constructor initializes
// a reference to the variable that needs to be captured.
template <class T>
struct DioSnapshotOnExitClass {
  const T& varref;
  const std::string& key;

  DioSnapshotOnExitClass(const T& _varref, const std::string& _key) :
    varref(_varref), key(_key) {};

  ~DioSnapshotOnExitClass() {
    DioGetOrSetSnapshot(1, key, &varref, nullptr);
  }
};

// Syntax sugar around DioSnapshotOnExitClass to take advantage of template
// function argument deduction. We can now write:
//
//   auto capturer = DioSnapshotOnExitFunction(somevar, "key");
//
// Note the lack of the type of 'somevar' above.
template <class T>
DioSnapshotOnExitClass<T> DioSnapshotOnExitFunction(const T& varname,
    const std::string& key) {
  return DioSnapshotOnExitClass<T>(varname, key);
}

// More syntax sugar to allow us to hide the explicit creation of
// a variable. We can now write:
//
//   DioSnapshotOnExit(somevar, "key");
//
// Note the lack of the type of 'somevar' above.
#define DioSnapshotOnExit(varname, key) auto capturer##varname = \
    DioSnapshotOnExitFunction(varname, key);

// Syntax sugar to snapshot a variable immediately. We can now write:
//
//   DioSnapshot(somevar, "key");
//
// Note the lack of the type of 'somevar' above.
#define DioSnapshot(varname, key) DioGetOrSetSnapshot(1 /* set */, key, \
    &(varname), nullptr);

#endif
