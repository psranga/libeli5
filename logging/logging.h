#ifndef MH312130ecb05661b3657063cde4152eb2b43527b8
#define MH312130ecb05661b3657063cde4152eb2b43527b8

constexpr int ERROR = 0;

constexpr int WARNING = 1;

constexpr int INFO = 2;

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

  ~NewLineAdder() { real_stream << endl; }
};
NewLineAdder GetLogger(int level, const char* filename, int line_num);

#define LOG(level) (GetLogger((level), __FILE__, __LINE__))

// Adds newlines. Also ignores stream op. if logging level is too low.
// Worth jumping through the hoops here so vlog statements that are expensive
// to execute are free, because the bypass the final stream output operator.
// So we can leave expensive debugging commands in the code, but pay little
// runtime penalty for this.
struct VlogNewLineAdder {
  std::ostream& real_stream;
  int level = 0;

  bool AmIActive() { return level <= vlog_level.get_flag(); }

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

  VlogNewLineAdder(std::ostream& stream, int _level, const char* filename,
                   int line)
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
VlogNewLineAdder GetVlogLogger(int level, const char* filename, int line_num);

#define VLOG(level) (GetVlogLogger((level), __FILE__, __LINE__))

#endif
