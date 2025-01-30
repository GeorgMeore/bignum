CFLAGS=-g -Wall -Wextra -fsanitize=undefined,address

main: bignum.o main.c
	cc $CFLAGS -o main main.c bignum.o

bignum.o: bignum.c bignum.h
	cc -c $CFLAGS -o bignum.o bignum.c
