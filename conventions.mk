# Compute the directory containing this file.
TOPDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# LibELI5 uses C++11 features.
CXXFLAGS+=-std=c++14 -Wall -Wno-sign-compare

# Link in logging library by default.
LDFLAGS+=-L$(TOPDIR)lib
LDFLAGS+=-Wl,-rpath,$(TOPDIR)lib

# Need this if-then-else because the logging 'Makefile' includes this, and we
# dont want to link in the logging lib when making test binaries in that dir.
ifeq ($(DONT_LINK_LOGGING),1)
else
LDLIBS+=-leli5_logging
endif

# Add directory containing headers. Using C++ code can do:
#   #incldue <eli5/file.h>
CXXFLAGS+=-I$(TOPDIR)include

# Utility that generates headers from a C++ source file that follows
# simple conventions.
MAKEHEADER=$(TOPDIR)cpp-makeheader/cpp-makeheader

# Make a header from a cc file automatically.
%.h : %.cc
	$(MAKEHEADER) < $< > $@

# Disable the pattern rule that builds a binary out of .o.
# We use the _main variant below.
# TODO(ranga): Think about removing the _main suffix for non-test binaries.
%: %.o
	@echo "Did you intend to build $@_main instead?" >&2
	@false

# Make binary out of a .cc file.
%_main :
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# Make a binary using the main() in diomain. Intended for header-only libraries
# written using ELI5 conventions: put the header stuff and test code into a
# single .cc file. Use 'cpp-makeheader' to generate the header file. And
# this _test_main target to run the library's tests.
#
# See flags/flags.cc for an example.
%_test_main :
	$(CXX) -o $@ $^ $(TOPDIR)diogenes/test_main.cc $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

# Include the ELI5 standard header by default in test binaries. This
# includes various standard C++ headers, and creates 'using' directives
# for commons classes like vector, map etc.
#
# I think eli5_stdlib.h is suitable for being implicitly included in
# any project. But since libeli5 is a library not a framework, it won't
# force this decision. A project that wants to implicitly include
# eli5_stdlib.h can add CXXFLAGS+=-include eli5/eli5_stdlib.h to
# its Makefile.
#
# Use the per-target variable feature of GNU Make to restrict this
# to test binaries.
%_test_main: CXXFLAGS+=-include eli5/eli5_stdlib.h

# Make fragment shader out of included fragment shaders.
%.out.frag :
	(echo '#version 330 core'; cpp $<  | grep -v '^# ') > $@
