CFLAGS=-g -Wall -Wextra -fsanitize=undefined,address

tests:V: test
	./test

test: bignum.o test.c
	cc $CFLAGS -o test test.c bignum.o

bignum.o: bignum.c bignum.h
	cc -c $CFLAGS -o bignum.o bignum.c
