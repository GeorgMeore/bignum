CFLAGS=-g -Wall -Wextra -fsanitize=undefined,address

tests:V: test
	./test

examples:V: examples/fact examples/gcd

examples/%: examples/%.c bignum.o
	cc -I . $CFLAGS -o $target bignum.o examples/$stem.c

test: bignum.o test.c
	cc $CFLAGS -o test test.c bignum.o

bignum.o: bignum.c bignum.h mkfile
	cc -c $CFLAGS -o bignum.o bignum.c
