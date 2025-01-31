#include <stdio.h>

#include "bignum.h"


void fact(Number x)
{
	Number acc = number(1);
	while (!iszero(x)) {
		mul(&acc, x);
		dec(&x, 1);
	}
	print16(acc);
	clear(&acc);
	clear(&x);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: %s NUM\n", argv[0]);
		return 1;
	}
	Number x = number(0);
	if (read(&x, argv[1])) {
		printf("error: not a valid number: %s\n", argv[1]);
		return 1;
	}
	fact(x);
	return 0;
}
