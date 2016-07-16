#ifndef MH98159821246739290431362639570744690946
#define MH98159821246739290431362639570744690946

// Eli5 command-line flags module.
//
// Complete Example:
//
//     #include "eli5/flags.h"
//
//     using eli5::define_flag;
//
//     define_flag<bool> verbosity("verbosity", 0);
//
//     int main(int argc, char* argv[]) {
//       eli5::InitializeFlags(argc, argv);
//       cout << "verbosity: " << verbosity.get_flag() << endl;
//     }
//
// If you dare, you can use the type-conversion operator to do:
//
//     if (verbosity > 3) {
//       cout << "ok" << endl;
//     }
//
// Here's how you set the flag value from you code, if you're into that
// sort of thing [and oh, you're bad and you should feel bad if you do this
// :)]
//
//     verbosity.set_flag(2);
//
// If you define the same flag in multiple source files, you'll get a link-time
// error. Make one of them an 'extern'. E.g.,
//
//     define_flag<bool> verbosity("verbosity", 0);
//
namespace eli5 {

// Base class for all flags: Maintains a (static) registry of all currently
// defined flags, that is updated from the constructor. Defines a pure virtual
// method 'set_flag' that parses a string and sets the flag value (this will be
// implemented in derived classes). The method set_flag will be used to set a
// flags value based on command-line parameter.
//
// The registry will be used the flags initization code to determine which
// flag's set_flag() method should be called for a certain command-line
// parameter.
struct basic_flag {
  // Name of the flag.
  string name;

  // Sets the flag by parsing the given string.
  virtual void set_flag(const string& s) = 0;

  // Workaround for C++ verbosity to keep things header-only. Wrap the static
  // registry in a static member function. If we made this a class-level
  // static, we would need a .cc file merely to define the variable.
  static vector<basic_flag*>& get_flags_registry() {
    static vector<basic_flag*> flags_registry;
    return flags_registry;
  }

  // Adds this flag to registry of all flags.
  basic_flag(const string& _name) : name(_name) {
    // Check that no flag with same name already exists.
    for (const auto& flag : get_flags_registry()) {
      assert(flag->name != name);
    }

    get_flags_registry().push_back(this);
  }
};
template <typename T>
struct FlagParser {
  T operator()(const string& s) { T::specialization_not_defined(); }
};
template <>
struct FlagParser<bool> {
  bool operator()(const string& s) { return (s == "1") || (s == "true"); }
};
template <>
struct FlagParser<int> {
  int operator()(const string& s) { return std::stoi(s); }
};
template <typename ValueType, typename FlagParserType = FlagParser<ValueType>>
struct define_flag : basic_flag {
  ValueType value;

  define_flag(const string& _name, const ValueType& _default_value)
      : basic_flag(_name), value(_default_value){};

  const ValueType& set_flag(const ValueType& new_value) {
    value = new_value;
    return get_flag();
  }

  void set_flag(const string& s) override {
    FlagParserType parser{};
    set_flag(static_cast<const ValueType&>(parser(s)));
  }

  const ValueType& get_flag() const { return value; }

  // Syntax sugar around get_flag. Can directly access the flag
  // as a const reference.
  operator const ValueType&() const { return get_flag(); }

  // Syntax sugar around set_flag.
  const ValueType& operator=(const ValueType& new_value) {
    return set_flag(new_value);
  }
};

// Call this at the start of main.
inline void InitializeFlags(int argc, const char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    string cmdparam(argv[i]);

    // Don't process command-line options after --. Convention is that they're
    // not meant for us.
    if (cmdparam == "--") {
      break;
    }

    // Flags are of the form: --flag=value.
    // Shortest valid flag is: --f=x
    if (cmdparam.size() < 5) {
      continue;
    }

    // Check if -- prefix is present.
    if (cmdparam.substr(0, 2) != "--") {
      continue;
    }

    auto split_pos = cmdparam.find('=');

    // Need to see the '='.
    if (split_pos == string::npos) {
      continue;
    }

    // Need at least one char after =.
    if (split_pos >= (cmdparam.size() - 1)) {
      continue;
    }

    // Extract the flag name and value parts from the command-line parameter.
    string flag_name = cmdparam.substr(2, split_pos - 2);
    string flag_value = cmdparam.substr(split_pos + 1);

    for (auto* flag : basic_flag::get_flags_registry()) {
      if (flag_name == flag->name) {
        flag->set_flag(flag_value);
      }
    }
  }
};
}
#endif
