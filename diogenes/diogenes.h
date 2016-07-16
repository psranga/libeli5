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

  // Makes a test out of any void lambda, and adds it to all tests.
  // This is the core idea of Diogenes.
  template <class Lambda>
  DioTest(Lambda l)
      : t(l) {
    AllTests().push_back(this);
  }

  // Runs this test. Test passes if none of the calls it makes to DioExpect
  // indicate failure. If it makes no calls, that means the test passes.
  void Run() const { t(); }

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

  // Runs all tests.
  static void RunAll() {
    for (DioTest* f : AllTests()) {
      f->Setup();
      f->Run();
      f->Teardown();
    }
    RecordExpectStatusOrPrintResults(false /* ignored */,
                                     false /* print status */);
  }

  // If 'record' is true, keeps track of the number of failing and
  // passing tests. If 'record' is false, print a report to
  // stdout saying how many passed and failed.
  static void RecordExpectStatusOrPrintResults(bool value, bool record) {
    static int passed = 0;
    static int failed = 0;
    if (record) {
      if (value) {
        ++passed;
      } else {
        ++failed;
      }
    } else {
      std::cout << "Diogenes results: " << passed << "/" << (passed + failed)
                << " expects passed (" << failed << " failed)." << std::endl;
      assert(failed == 0);
    }
  }

  // Prefer the macro DioExpect below.
  static void DioExpect2(const std::string& expression_str, bool value) {
    RecordExpectStatusOrPrintResults(value, true /* record */);
    if (!value) {
      std::cerr << "Failed test: '" << expression_str << "'" << std::endl;
    }
  }
};

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
