include ../conventions.mk

all: variant.cc variant.h variant_test_main

variant.cc: variant.gen.cc
	../bin/py/bin/python genvariant.py < variant.gen.cc  > variant.cc

variant.h: variant.cc
	../cpp-makeheader/cpp-makeheader < variant.cc > variant.h

clean:
	rm -f variant.cc variant.h variant_test_main
