// Simple logging to cout with two major conveniences:
//   1. newlines are added automatically (no need for explicit endls).
//   2. Verbose debug log statements will be efficiently ignored even if
//      present in the code.
//
// Example:
//
// Unconditionally log to cout:
// LOG(INFO) << "abcd" << ':' << ' ' << 1234;
//
// Log to cout if command-line flag vlog_level is >= 3.
// VLOG(3) << "abcd" << ':' << ' ' << 1234;

constexpr int INFO = 0;

// RAII to automatically add a newline at the end of a log command like
//   LOG(INFO) << "abcd" << "1234";
struct NewLineAdder {
  std::ostream& real_stream;

  // Forward all stream output operations to the real stream.
  template <typename T>
  std::ostream& operator<<(const T& t) {
    return (real_stream << t);
  }

  NewLineAdder(std::ostream& stream) : real_stream(stream) {}

  ~NewLineAdder() {
    real_stream << endl;
  }
};

NewLineAdder GetLogger(int level, const char* filename, int line_num) {
  cout << filename << ':' << line_num << ": ";
  return NewLineAdder(cout);
}

#define LOG(level) (GetLogger((level), __FILE__, __LINE__))

static DioTest Test_GetLogger = []() {
  (GetLogger(0, "foo.cc", 0)) << "Hello world";
  (GetLogger(0, "foo.cc", 1)) << "Hello world";
};

static DioTest Test_NewLineAdder = []() {
  cout << "================" << endl;
  NewLineAdder(cout) << "hello world" << "1234";
  cout << "after hello world" << endl;
};

static DioTest Test_LOG_INFO = []() {
  LOG(INFO) << "Hello world";
  LOG(INFO) << "Hello world 2";
};

//extern define_flag<bool> vlog_level("vlog_level", 0);
define_flag<bool> vlog_level("vlog_level", 0);

// Adds newlines. Also ignores stream op. if logging level is too low.
// Worth jumping through the hoops here so vlog statements that are expensive
// to execute are free, because the bypass the final stream output operator.
// So we can leave expensive debugging commands in the code, but pay little
// runtime penalty for this.
struct VlogNewLineAdder {
  std::ostream& real_stream;
  int level = 0;

  bool AmIActive() {
   return level <= vlog_level.get_flag();
  }

  // Forward all stream output operations to the real stream.
  template <typename T>
  std::ostream& operator<<(const T& t) {
    if (AmIActive()) {
      return (real_stream << t);
    } else {
      // Don't do the logging operation. This is very efficient since any
      // expensive operations to convert t to string form will be bypassed.
      return real_stream;
    }
  }

  VlogNewLineAdder(std::ostream &stream, int _level, const char *filename, int line)
      : real_stream(stream), level(_level) {
    if (AmIActive()) {
      real_stream << filename << ':' << line << ": ";
    }
  }

  ~VlogNewLineAdder() {
    // Add a newline if we logged anything.
    if (AmIActive()) {
      real_stream << endl;
    }
  }
};

VlogNewLineAdder GetVlogLogger(int level, const char* filename, int line_num) {
  return VlogNewLineAdder(cout, level, filename, line_num);
}

#define VLOG(level) (GetVlogLogger((level), __FILE__, __LINE__))

static DioTest Test_Vlog = []() {
  VLOG(0) << "Hello vlog0.";
  VLOG(1) << "Hello vlog1.";
  VLOG(0) << "Hello vlog00.";
  vlog_level.set_flag(1);
  VLOG(1) << "Hello vlog11.";
};
