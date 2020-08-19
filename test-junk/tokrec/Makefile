CFLAGS=-Wall -std=c99 -DNLEX_ITSELF

default:lexer

lexer:main.c ../../src/read.o ../../src/*.h
	cc $(CFLAGS) -o lexer ../../src/read.o main.c -I../../src

clean:
	rm -f lexer
