# Compute the directory containing this file.
TOPDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# LibELI5 uses C++11 features.
CXXFLAGS+=-std=c++14

# Add directory containing headers. Using C++ code can do:
#   #incldue <eli5/file.h>
CXXFLAGS+=-I$(TOPDIR)include

# Utility that generates headers from a C++ source file that follows
# simple conventions.
MAKEHEADER=$(TOPDIR)cpp-makeheader/cpp-makeheader

# Make a header from a cc file automatically.
%.h : %.cc
	$(MAKEHEADER) < $< > $@

# Make binary out of a .cc file.
%_main : %.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# Make a binary using the main() in diomain. Intended for header-only libraries
# written using ELI5 conventions: put the header stuff and test code into a
# single .cc file. Use 'cpp-makeheader' to generate the header file. And
# this _test_main target to run the library's tests.
#
# See flags/flags.cc for an example.
%_test_main : %.cc
	$(CXX) -o $@ $^ $(TOPDIR)diogenes/test_main.cc $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

# Include the ELI5 standard header by default in test binaries. This
# includes various standard C++ headers, and creates 'using' directives
# for commons classes like vector, map etc.
#
# Using the per-target variable feature of GNU Make.
%_test_main: CXXFLAGS+=-include eli5/eli5_stdlib.h
