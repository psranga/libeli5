include ../conventions.mk

all: flags.h flags_test_main

flags_test_main: CXXFLAGS+=-DDONT_INCLUDE_FLAGS

clean:
	rm -f flags.h flags_test_main
