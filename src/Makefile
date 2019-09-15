CFLAGS=-Wall -std=c99 -DNLEX_ITSELF
DEBUGFLAGS=-D DEBUG_MODE -g

# Use `make clean; make debug=1` to turn on the debug mode.
ifdef debug
  CFLAGS += $(DEBUGFLAGS)
else
  CFLAGS += -s # strip
endif

default:lexgen

%.o: %.c *.h
	cc $(CFLAGS) -c $<

lexgen:main.o read.o
	cc $(CFLAGS) -o lexgen main.o read.o

clean:
	rm -f *.o lexgen
