export AR = ar
export RANLIB = ranlib
export CC = gcc
export CXX = g++
export LD = g++
export CFLAGS = -O0 -g -fsanitize=address,undefined
export CXXFLAGS = $(CFLAGS) -std=c++11 -Wall -Werror -DLOG_LEVEL=LOG_LEVEL_INFO
export LDFLAGS = -fsanitize=address,undefined

INCLUDE = -I../lib/libmad -I./arduino_stub -I../src
LIBS = -L./libmad -L./arduino_stub -larduino_stub -lmad

BINARIES = decode_mp3 decode_mp3_dir
LIBRARIES = arduino_stub/libarduino_stub.a libmad/libmad.a
SOURCE = MadDecoder.cxx DirectoryPlayer.cxx DirectoryReader.cxx
OBJECTS = $(SOURCE:.cxx=.o)

all: sub_all
	$(MAKE) -C. binaries

binaries: $(BINARIES)

$(BINARIES) : % : %.cxx $(OBJECTS) $(LIBRARIES)
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o $@ $< $(OBJECTS)

$(SOURCE:.cxx=.o) : %.o : ../src/%.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

clean: sub_clean
	rm -f $(OBJECTS) $(LIBRARY)

sub_clean:
	$(MAKE) -C arduino_stub clean
	$(MAKE) -C libmad clean

sub_all:
	$(MAKE) -C arduino_stub all
	$(MAKE) -C libmad all

.PHONY: all clean sub_clean sub_all
