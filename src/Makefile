CFLAGS=-Wall -std=c99 -DNLEX_ITSELF

default:lexgen

%.o: %.c *.h
	cc $(CFLAGS) -c $<

lexgen:main.o read.o
	cc $(CFLAGS) -o lexgen main.o read.o

clean:
	rm -f *.o lexgen
