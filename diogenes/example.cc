// DioTest: Simple Unit Test Framework
// Purplerails <ranga@purplerails.com>
//
// Released under the GPL version 2.

#include "diogenes.h"
#include <iostream>

using std::cout;
using std::endl;

// The function being tested.
static void ClearValue(int& i) {
  int localvar = i + 2;

  // Snapshot internal state for use in the unit test code. See below.
  DioSnapshot(localvar, "immediate");
  DioSnapshotOnExit(localvar, "on_exit");

  i = 0;

  localvar += 2;
}

// Testing the function above:
// Creata lambda containing your test and assign it to
// a variable of type DioTest.
static DioTest Test_ClearInt = []() {
  // Set up inputs.
  int i = 12;

  // Call the function being tested.
  ClearValue(i);

  // Check that the function worked.
  DioExpect(i == 0);

  // Also check the internal state that was snapshotted.
  DioExpect(DioGetSnapshottedValue<int>("immediate") == 14);

  // Note the lack of 'int' by passing an unused parameter 'i'. This
  // style should probably be used sparingly.
  DioExpect(DioGetSnapshottedValue("on_exit", i) == 16);

  cout << "immediate: " <<
    DioGetSnapshottedValue<int>("immediate") << endl;
  cout << "on exit: " <<
    DioGetSnapshottedValue<int>("on_exit") << endl;
};

int main() {
  // Call this somewhere to run all tests in all files.
  Diogenes::RunAll();
}
