CFLAGS=-Wall -std=c99 -DNLEX_ITSELF
DEBUGFLAGS=-D DEBUG_MODE -g

# Use `make clean; make debug=1` to turn on the debug mode.
ifdef debug
  CFLAGS += $(DEBUGFLAGS)
else
  CFLAGS += -s # strip
endif

default:nlexgen

%.o: %.c *.h
	cc $(CFLAGS) -c $<

nlexgen:main.o read.o
	cc $(CFLAGS) -o nlexgen main.o read.o

clean:
	rm -f *.o nlexgen
