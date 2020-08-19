CFLAGS=-Wall -std=c99

default:lexer

lexbranch.c: in
	cat in|../../src/nlexgen > lexbranch.c

lexer:in main.c ../../src/read.o ../../src/*.h lexbranch.c
	cc $(CFLAGS) -o lexer ../../src/read.o main.c -I../../src

clean:
	rm -f lexbranch.c lexer
