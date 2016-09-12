#include "eli5/flags.h"

using eli5::define_flag;

define_flag<bool> verbosity("verbosity", 0);
define_flag<int> numeric_flag("numeric_flag", 0x31412);
