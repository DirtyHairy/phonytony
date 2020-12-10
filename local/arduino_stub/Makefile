AR ?= ar
RANLIB ?= ranlib
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O0 -g -fsanitize=undefined,address
CXXFLAGS ?= $(CFLAGS) -std=c++11 -Wall -Werror

INCLUDE = -I. -I../../src/fs

SOURCE_C = \
	stdlib_noniso.c

SOURCE_CC = \
	esp_log.cxx \
	esp_stubs.cxx \
	hal_stub.cxx \
	WString.cxx \
	Stream.cxx \
	Print.cxx \
	FS.cxx \
	../../src/fs/PosixFile.cxx \
	Serial.cxx

OBJECTS = $(SOURCE_C:.c=.o) $(SOURCE_CC:.cxx=.o)
LIBRARY = libarduino_stub.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) cru $@ $^
	$(RANLIB) $@

$(SOURCE_C:.c=.o): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(SOURCE_CC:.cxx=.o): %.o : %.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	-rm $(OBJECTS) $(LIBRARY)

.PHONY: all clean
