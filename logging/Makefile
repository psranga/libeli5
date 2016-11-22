DONT_LINK_LOGGING=1
include ../conventions.mk

all: logging.h logging_test_main libeli5_logging.a

logging_test_main: CXXFLAGS+=-DDONT_INCLUDE_LOGGING
logging_test_main: logging.cc

logging.o: CXXFLAGS+=-DDONT_INCLUDE_LOGGING
logging.o: CXXFLAGS+=-include eli5/eli5_stdlib.h

libeli5_logging.a: logging.o
	ar rcs $@ $<

clean:
	rm -f logging.h logging_test_main libeli5_logging.a logging.o
