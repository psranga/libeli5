#include "eli5/flags.h"

using eli5::define_flag;

extern define_flag<bool> verbosity;
extern define_flag<int> numeric_flag;

static DioTest Test_FlagSharing = []() {
  cout << "verbosity: " << verbosity.get_flag() << endl;
  DioExpect(!verbosity.get_flag());

  verbosity.set_flag(false);
  cout << "verbosity: " << verbosity.get_flag() << endl;
  DioExpect(!verbosity.get_flag());

  verbosity.set_flag(true);
  cout << "verbosity: " << verbosity.get_flag() << endl;
  DioExpect(verbosity.get_flag());
};

static DioTest Test_FlagSharingInt = []() {
  cout << "value: " << numeric_flag.get_flag() << endl;
  DioExpect(numeric_flag.get_flag() == 0x31412);

  numeric_flag.set_flag(100);
  cout << "value: " << numeric_flag.get_flag() << endl;
  DioExpect(numeric_flag.get_flag() == 100);

};
