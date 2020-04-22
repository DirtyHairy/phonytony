AR ?= ar
RANLIB ?= ranlib
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O0 -g
CXXFLAGS ?= $(CFLAGS) -std=c++11

SOURCE_C = stdlib_noniso.c
SOURCE_CC = WString.cpp
OBJECTS = $(SOURCE_C:.c=.o) $(SOURCE_CC:.cpp=.o)
LIBRARY = libarduino_stub.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) cru $@ $^
	$(RANLIB) $@

$(SOURCE_C:.c=.o): %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(SOURCE_CC:.cpp=.o): %.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm $(OBJECTS) $(LIBRARY)

.PHONY: all clean
