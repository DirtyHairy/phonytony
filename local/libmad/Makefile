AR ?= ar
RANLIB ?= ranlib
CC ?= gcc
CFLAGS ?= -O0 -g -fsanitize=undefined,address

INCLUDE = -I../../lib/libmad -I../arduino_stub

SOURCE = bit.c decoder.c fixed.c frame.c huffman.c layer3.c stream.c synth.c timer.c version.c
OBJECTS = $(SOURCE:.c=.o)
LIBRARY = libmad.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) cru $@ $^
	$(RANLIB) $@

$(OBJECTS): %.o : ../../lib/libmad/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(LIBRARY)

.PHONY: all clean
