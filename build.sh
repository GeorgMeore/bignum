#!/bin/sh -ex

CFLAGS='-g -Wall -Wextra -fsanitize=undefined,address'

cc $CFLAGS -o bignum bignum.c
