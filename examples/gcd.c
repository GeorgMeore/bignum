#include <stdio.h>
#include "bignum.h"


#define SWAP(a, b) ({typeof(a) tmp = b; b = a; a = tmp;})

void gcd(Number a, Number b)
{
	while (!iszero(a)) {
		rem(&b, a);
		SWAP(a, b);
	}
	print10(b);
	clear(&a);
	clear(&b);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("usage: %s NUM NUM\n", argv[0]);
		return 1;
	}
	Number a = number(0), b = number(0);
	if (read(&a, argv[1])) {
		printf("error: not a valid number: %s\n", argv[1]);
		return 1;
	}
	if (read(&b, argv[2])) {
		printf("error: not a valid number: %s\n", argv[2]);
		clear(&a);
		return 1;
	}
	gcd(a, b);
}
