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

#include <sstream>

constexpr int ERROR = 0;
constexpr int WARNING = 1;
constexpr int INFO = 2;
constexpr int MEMORY = 3;

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


// Gymnastics to keep this header-only.
std::stringstream* InMemoryLogger(int operation = 0) {
  static std::stringstream* in_memory_logger = nullptr;

  if (in_memory_logger == nullptr) {
    in_memory_logger = new std::stringstream();
  }

  if (operation == 0) {
    // return current value.
  } else if (operation == 1) {
    // Clear log.
    in_memory_logger->str(string());
  }
  return in_memory_logger;
}

NewLineAdder GetLogger(int level, const char* filename, int line_num) {
  std::ostream& os =
      (level == MEMORY) ? (*InMemoryLogger()) : (level == 2) ? cout : cerr;
  os << filename << ':' << line_num << ": ";
  return NewLineAdder(os);
}

#define LOG(level) (GetLogger((level), __FILE__, __LINE__))

static DioTest Test_GetLogger = []() {
  (GetLogger(MEMORY, "foo.cc", 0)) << "Hello world";
  (GetLogger(MEMORY, "foo.cc", 1)) << "Hello world";
};

static DioTest Test_NewLineAdder = []() {
  std::stringstream os;
  os << "================" << endl;
  NewLineAdder(os) << "hello world" << "1234";
  os << "after hello world" << endl;
  DioExpect(os.str() == "================\nhello world1234\nafter hello world\n");
};

static DioTest Test_LOG_MEMORY = []() {
  InMemoryLogger(1);  // Clear logs.
  LOG(MEMORY) << "Hello world";
  LOG(MEMORY) << "Hello world 2";
};

// Log everything at INFO and below.
define_flag<bool> vlog_level("vlog_level", 2);

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
  VlogNewLineAdder& operator<<(const T& t) {
    if (AmIActive()) {
      real_stream << t;
    } else {
      // Don't do the logging operation. This is very efficient since any
      // expensive operations to convert t to string form will be bypassed.
    }
    return *this;
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

VlogNewLineAdder GetMlogLogger(int level, const char* filename, int line_num) {
  return VlogNewLineAdder(*InMemoryLogger(), level, filename, line_num);
}

#define VLOG(level) (GetVlogLogger((level), __FILE__, __LINE__))

#define MLOG(level) (GetMlogLogger((level), __FILE__, __LINE__))

static DioTest Test_Vlog = []() {
  InMemoryLogger(1);  // Clear log.
  MLOG(0) << "Hello vlog0.";
  MLOG(1) << "Hello vlog1.";
  MLOG(0) << "Hello vlog00.";
  vlog_level.set_flag(1);
  MLOG(1) << "Hello vlog11.";
};
