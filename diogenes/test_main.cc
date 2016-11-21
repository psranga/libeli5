int main(int argc, char** argv) {
#if !defined(DONT_INCLUDE_FLAGS)
  eli5::InitializeFlags(argc, argv);
#endif
  Diogenes::RunAll();
}
