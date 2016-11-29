#include <iostream>
#include "diogenes.h"
#include "eli5/flags.h"

using std::cout;
using std::endl;

void ClearValue(int& i) {
  i = 0;
}

DIOTEST(Test_ClearInt4) = []() {
  cout << "running!" << endl;
  // Set up inputs.
  int i = 12;

  // Call the function being tested.
  ClearValue(i);

  // Check that the function worked.
  DioExpect(i == 0);
};

#include "test_main.cc"
