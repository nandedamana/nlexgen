CFLAGS=-Wall -std=c99 -DNLEX_ITSELF

default:lexer

lexer:in main.c ../../src/read.o ../../src/*.h
	cat in|../../src/nlexgen > lexbranch.c
	cc $(CFLAGS) -o lexer ../../src/read.o main.c -I../../src

clean:
	rm -f lexbranch.c lexer
