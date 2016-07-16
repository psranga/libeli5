Diogenes: World's Simplest Unit Testing "Framework"?
Purplerails <ranga@purplerails.com>

# Introduction

Diogenes is the unit testing framework from the "worse is better"
school of thought.
https://en.wikipedia.org/wiki/Worse_is_better

C++ unit testing "feels" more complicated that it should be. Do we really need to learn
"frameworks" to do something conceptually simple like unit testing?

# Example

    // g++ -std=c++11 -Wall example.cpp
    #include "diogenes.h"

    // The function being tested.
    void ClearValue(int& i) {
      i = 0;
    }

    // Create a unit test by writing a lambda function and assigning it to
    // a variable of type DioTest.
    static DioTest Test_ClearInt = []() {
      // Set up inputs.
      int i = 12;

      // Call the function being tested.
      ClearValue(i);

      // Check that the function worked.
      DioExpect(i == 0);
    };

    int main() {
      // Call this somewhere to run all tests in all files.
      Diogenes::RunAll();
    }

# Core Ideas

* Tests should be placed next code in the same file. We consider tests equal in
  value to comments and code. It makes sense to put them next to code, so that
  tests can be quickly added/modified along with the code.

* Should be learnable in five minutes or less. You have you real problem to
  solve and respect your time. I understand that Diogenes is a means to an end,
  not an end itself.

* Simple implementation. You should be able to fix things if needed.

* Easy to use in your projects. Pulling in Diogenes should not result in your
  having to pull in lots of other code dependencies, or restructure your code
  in new ways. Diogenes achieves this by being header-only.

# Details

## The first 80%: Simple tests

The example shown above should suffice for the vast majority of the code you
write. It's good practice to keep various parts of your code decoupled anyway,
so simple Diogenes style of unit testing should take you quite far.

Note that there no explicit setup/teardown for each test, so the tests should
be independent of each other.

## Setup/Teardown of tests

In some cases, you may want to perform the initialization and cleanup at the
beginning of multiple tests.  They may be some benefit to having the testing
framework do this for you instead of explicity calling the setup/teardown
functions yourself within every unit test.

To do this, derive a class, say MyTest, from DioTest and override the Setup()
and Teardown() methods.  Now you can assign lambdas to MyTest. Except that the
Setup() and Teardown() functions will be called before running these lambdas.

But ... due to a small wrinkle in C++, you'll have to write the boilerplate
'using DioTest::DioTest' as the first line of your derived class. This makes
the constructor that allows you to "assign" lambdas available to your derived
class. I feel that this is a more "honest" design than declaring a preprocessor
macro that effectively does this.

### Example

    struct MyTest : DioTest {
      using DioTest::DioTest;

      void Setup() {
      }

      void Teardown() {
      }
    };

    static MyTest Test_Something = []() {
      // Do stuff.
    };

## Experimental: Snapshotting variables in test mode

If you want to full honey badger and ignore "rules" like testing only through
final outputs, Diogenes can help you export internal state of the code under
test for inspection in the unit test.

This is implemented by *copying* internal variables into a typesafe in-memory
key-value store. Use getters to examine the captured variables like any other.

There's some syntax sugar to enable capturing a variable when exiting a block.
This is implemented using the standard execute-in-destructor pattern.

### Example

    // g++ -std=c++11 -Wall example.cpp
    #include "diogenes.h"

    // The function being tested.
    static void ClearValue(int& i) {
      int localvar = i + 2;

      // Capture a copy of localvar immediately, and give it the key
      // "immediate" to retrieve it.
      DioSnapshot(localvar, "immediate");

      // Capture a copy of localvar at the end of scope containing this
      // directive (not at the end of the scope containing the variable),
      // and give it the key "on_exit" to retrieve it.
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

      // Check the value at end of scope.
      DioExpect(DioGetSnapshottedValue<int>("on_exit") == 16);
    };

    int main() {
      // Call this somewhere to run all tests in all files.
      Diogenes::RunAll();
    }

