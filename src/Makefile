default:lexgen

lexgen:main.o read.o
	cc -o lexgen main.o read.o

test1:lexgen test/1.in
	cat test/1.in|./lexgen
