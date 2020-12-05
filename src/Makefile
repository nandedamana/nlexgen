CFLAGS=-Wall -std=c99 -DNLEX_ITSELF
DEBUGFLAGS=-D DEBUG_MODE -g
OBJS=error.o main.o read.o tree.o

# Use `make clean; make debug=1` to turn on the debug mode.
ifdef debug
  CFLAGS += $(DEBUGFLAGS)
else
  CFLAGS += -s # strip
endif

default:nlexgen

error.c: errmap.tsv error.c.top
	cp error.c.top error.c
	awk -F "\t" '{print "const char * "$$1" = "$$2";"}' errmap.tsv >> error.c

error.h: errmap.tsv error.h.top
	cp error.h.top error.h
	awk -F "\t" '{print "extern const char * "$$1";"}' errmap.tsv >> error.h
	echo '#endif' >> error.h

%.o: %.c *.h error.h
	cc $(CFLAGS) -c $<

nlexgen:$(OBJS)
	cc $(CFLAGS) -o nlexgen $(OBJS)

clean:
	rm -f nlexgen *.o error.[ch]
