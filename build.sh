#!/bin/sh -ex

CFLAGS="-g -Wall -Wextra -fsanitize=undefined,address"

cc -c $CFLAGS -o bignum.o bignum.c
cc $CFLAGS -o test test.c bignum.o &
cc -I . $CFLAGS -o examples/fact bignum.o examples/fact.c &
cc -I . $CFLAGS -o examples/gcd bignum.o examples/gcd.c &
wait
./test
