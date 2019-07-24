default:lexgen

%.o: %.c *.h
	cc -c $<

lexgen:main.o read.o
	cc -o lexgen main.o read.o

1test:lexgen test1/in test1/main.c read.o
	mkdir -p test1/out/
	cat test1/in|./lexgen > test1/out/lexbranch.c
	cc -o test1/out/lexer read.o test1/main.c -I.
	
2test:lexgen test2/*.in test2/main.c read.o
	mkdir -p test2/out/
	cat test2/default.in|./lexgen > test2/out/lexbranch-default.c
	cat test2/comment.in|./lexgen > test2/out/lexbranch-comment.c
	cat test2/string.in|./lexgen > test2/out/lexbranch-string.c
	cc -o test2/out/lexer read.o test2/main.c -I.

clean:
	rm -f *.o lexgen
