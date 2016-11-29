// A test runnner for Diogenes. Suitable for use in external projects.
// Can be used inside ELI5 unless this test runner uses the module
// that is being tested. A different, simpler test runner should be
// used in those cases.
//
// Until I get around using different test runners, the ugly define
// checks are used to disable features when they are declared as
// unavailable.
#if !defined(DONT_INCLUDE_FLAGS)
define_flag<string> diofilter("diofilter", "");
#endif

int main(int argc, char** argv) {
#if !defined(DONT_INCLUDE_FLAGS)
  eli5::InitializeFlags(argc, argv);
#endif
#if !defined(DONT_INCLUDE_LOGGING)
  if (!diofilter.get_flag().empty()) {
    LOG(INFO) << "Running a subset of tests: " << diofilter.get_flag();
  } else {
    LOG(INFO) << "Running all tests.";
  }
#endif
#if !defined(DONT_INCLUDE_FLAGS)
  Diogenes::RunAll(string(diofilter.get_flag()));
#else
  // No flags. Run all tests.
  Diogenes::RunAll();
#endif
}
