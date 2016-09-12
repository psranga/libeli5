include ../conventions.mk

all: flags.h flags_test_main share_flags_test_main

flags_test_main: CXXFLAGS+=-DDONT_INCLUDE_FLAGS

share_flags_test_main: definer.cc

clean:
	rm -f flags.h flags_test_main share_flags_test_main
