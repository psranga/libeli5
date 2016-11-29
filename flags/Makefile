DONT_LINK_LOGGING=1
include ../conventions.mk

all: flags.h flags_test_main share_flags_test_main

flags_test_main: CXXFLAGS+=-DDONT_INCLUDE_FLAGS -DDONT_INCLUDE_LOGGING
flags_test_main: flags.cc

share_flags_test_main: CXXFLAGS+=-DDONT_INCLUDE_FLAGS -DDONT_INCLUDE_LOGGING
share_flags_test_main: share_flags.cc
share_flags_test_main: definer.cc

clean:
	rm -f flags.h flags_test_main share_flags_test_main
