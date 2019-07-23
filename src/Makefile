default:lexgen

lexgen:main.o read.o
	cc -o lexgen main.o read.o

test:lexgen test1/in test1/main.c read.o
	mkdir -p test1/out/
	cat test1/in|./lexgen > test1/out/lexbranch.c
	cc -o test1/out/lexer read.o test1/main.c -I.
