CFLAGS=-Wall -std=c99 -DNLEX_ITSELF
DEBUGFLAGS=-DDEBUG -g
OBJS=error.o plot.o main.o read.o tree.o

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

types.h: types.ngg
	ngg -oh types.h -oc /dev/null --c-include-guard _N96E_LEX_TYPES_H types.ngg

%.o: %.c *.h types.h error.h
	cc $(CFLAGS) -c $<

nlexgen:$(OBJS)
	cc $(CFLAGS) -o nlexgen $(OBJS)

clean:
	rm -f nlexgen *.o error.[ch] types.h
