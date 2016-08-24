#ifndef ELI5_STDLIB_
#define ELI5_STDLIB_

#include <array>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using std::array;
using std::ceil;
using std::cerr;
using std::cout;
using std::endl;
using std::make_pair;
using std::make_unique;
using std::map;
using std::move;
using std::pair;
using std::size_t;
using std::string;
using std::tie;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

#include "eli5/diogenes.h"

#if !defined(DONT_INCLUDE_FLAGS)
#include "eli5/flags.h"
using eli5::define_flag;
#endif

#endif
