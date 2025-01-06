bignum: bignum.c
	cc -g -Wall -Wextra -fsanitize=undefined,address -o $target $prereq
