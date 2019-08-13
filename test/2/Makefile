CFLAGS=-Wall -std=c99 -DNLEX_ITSELF

default:out/lexer

out/lexer:*.in main.c ../../src/read.o
	mkdir -p out
	cat default.in|../../src/lexgen > out/lexbranch-default.c
	cat comment.in|../../src/lexgen > out/lexbranch-comment.c
	cat string.in|../../src/lexgen > out/lexbranch-string.c
	cc $(CFLAGS) -o out/lexer ../../src/read.o main.c -I../../src

clean:
	rm -f out/*
