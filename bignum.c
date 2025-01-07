#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef struct {
	uint len;
	uint cap;
	ulong *d;
	uchar neg;
} Number;

Number number(ulong n)
{
	Number a;
	a.len = 1;
	a.cap = 16; /* the default initial capacity */
	a.d = calloc(a.cap, sizeof(a.d[0]));
	a.d[0] = n; /* all others are zeroed by calloc */
	return a;
}

Number copy(Number n)
{
	Number c;
	c.len = n.len;
	c.cap = n.cap;
	c.d = calloc(c.cap, sizeof(c.d[0]));
	for (uint i = 0; i < c.len; i++)
		c.d[i] = n.d[i];
	c.neg = n.neg;
	return c;
}

void clear(Number *n)
{
	free(n->d);
	n->cap = 0;
	n->len = 0;
	n->d = 0;
}

void neg(Number *n)
{
	n->neg = !n->neg;
}

void zero(Number *n)
{
	n->neg = 0;
	for (uint i = 0; i < n->len; i++)
		n->d[i] = 0;
	n->len = 1;
}

int iszero(Number n)
{
	return !n.len || (n.len == 1 && n.d[0] == 0);
}

static void extend(Number *n, uint chunks)
{
	n->len += chunks;
	if (n->len <= n->cap)
		return;
	while (n->len > n->cap)
		n->cap = n->len * 2;
	n->d = reallocarray(n->d, n->cap, sizeof(n->d[0]));
	for (uint i = n->len - chunks; i < n->cap; i++)
		n->d[i] = 0;
}

static void absadd(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int carry = 0;
	for (uint i = 0; i < src.len; i++) {
		dst->d[i] += src.d[i] + carry;
		carry = (carry && !(src.d[i] + carry)) || dst->d[i] < src.d[i] + carry;
	}
	for (uint i = src.len; i < dst->len && carry; i++) {
		dst->d[i] += carry;
		carry = !dst->d[i];
	}
	if (carry) {
		extend(dst, 1);
		dst->d[dst->len-1] = 1;
	}
}

static void shrink(Number *n)
{
	while (n->len > 1 && !n->d[n->len-1])
		n->len -= 1;
}

static void flip(Number *n)
{
	for (uint i = 0; i < n->len; i++)
		n->d[i] = ~n->d[i];
	n->d[0] += 1;
}

void abssub(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int borrow = 0;
	for (uint i = 0; i < src.len; i++) {
		dst->d[i] -= src.d[i] + borrow;
		borrow = (borrow && !(src.d[i] + borrow)) || dst->d[i] > ~(src.d[i] + borrow);
	}
	for (uint i = src.len; i < dst->len && borrow; i++) {
		dst->d[i] -= borrow;
		borrow = !~dst->d[i];
	}
	if (borrow) {
		neg(dst);
		flip(dst);
	}
	shrink(dst);
}

static int abscmp(Number a, Number b)
{
	if (a.len > b.len)
		return 1;
	if (a.len < b.len)
		return -1;
	for (int i = a.len - 1; i >= 0; i--) {
		if (a.d[i] > b.d[i])
			return 1;
		if (a.d[i] < b.d[i])
			return -1;
	}
	return 0;
}

int cmp(Number a, Number b)
{
	if (iszero(a) && iszero(b))
		return 0;
	if (a.neg == b.neg) {
		if (a.neg)
			return -abscmp(a, b);
		else
			return abscmp(a, b);
	} else {
		if (a.neg)
			return -1;
		else
			return 1;
	}
}

void add(Number *dst, Number src)
{
	if (dst->neg == src.neg)
		absadd(dst, src);
	else
		abssub(dst, src);
}

void sub(Number *dst, Number src)
{
	neg(&src);
	add(dst, src);
}

void rshift(Number *n, uint bits)
{
	uint perchunk = sizeof(n->d[0])*8;
	uint drop = bits / perchunk;
	if (drop >= n->len) {
		zero(n);
		return;
	}
	if (drop) {
		for (uint i = 0; i < n->len-drop; i++) {
			n->d[i] = n->d[i+drop];
			n->d[i+drop] = 0;
		}
	}
	uint shift = bits % perchunk;
	if (shift) {
		ulong mask = (1UL << shift) - 1;
		n->d[0] >>= shift;
		for (uint i = 0; i < n->len-drop-1; i++) {
			n->d[i] |= (n->d[i+1] & mask) << (perchunk - shift);
			n->d[i+1] >>= shift;
		}
	}
	shrink(n);
}

void lshift(Number *n, uint bits)
{
	uint perchunk = sizeof(n->d[0])*8;
	uint move = bits / perchunk;
	if (move) {
		extend(n, move);
		for (uint i = 0; i < n->len-move; i++) {
			n->d[n->len-1-i] = n->d[n->len-1-i - move];
			n->d[n->len-1-i - move] = 0;
		}
	}
	uint shift = bits % perchunk;
	if (shift) {
		ulong mask = ((1UL << shift) - 1) << (perchunk - shift);
		if (n->d[n->len-1] & mask)
			extend(n, 1);
		else
			n->d[n->len-1] <<= shift;
		for (uint i = 0; i < n->len-move-1; i++) {
			n->d[n->len-i-1] |= (n->d[n->len-i-2] & mask) >> (perchunk - shift);
			n->d[n->len-i-2] <<= shift;
		}
	}
}

void read(Number *n, char *s)
{
	zero(n);
	if (*s == '-') {
		n->neg = 1;
		s++;
	}
	uint perchunk = sizeof(n->d[0]) * 2;
	uint slen = strlen(s);
	uint last = (slen + perchunk - 1) / perchunk;
	if (last >= n->len)
		extend(n, last + 1 - n->len);
	for (uint i = 0; i < slen; i++) {
		char c = s[slen-1-i];
		ulong digit;
		if (c >= '0' && c <= '9') {
			digit = c - '0';
		} else if ((c|32) >= 'a' && (c|32) <= 'f') {
			digit = (c|32) - 'a' + 10;
		} else {
			printf("error: not a hex digit: %c\n", c);
			return;
		}
		int chunk = i / perchunk;
		int shift = i % perchunk;
		n->d[chunk] += digit << (shift * 4);
	}
	shrink(n);
}

void print(Number n)
{
	if (n.neg && !iszero(n))
		printf("-");
	printf("0x%lx", n.d[n.len-1]);
	for (uint i = 1; i < n.len; i++)
		printf("%016lx", n.d[n.len-1-i]);
	printf("\n");
}

int main(void)
{
	Number a = number(0);
	read(&a, "123456789abcdefedcba9876543210");
	lshift(&a, 113);
	print(a);
	rshift(&a, 99);
	print(a);
	clear(&a);
	return 0;
}

int main2(int, char **)
{
	Number a = number(0);
	Number b = number(0);
	Number z = number(0);
	read(&a, "123456789abcdefedcba9876543210");
	read(&b, "912374198327491832749817348971298374981273498719283749817234987129384791234981237498123749875192357192381423");
	print(a);
	neg(&a);
	print(a);
	for (int i = 0; i < 100000; i++)
		add(&a, b);
	print(a);
	for (int i = 0; i < 100000; i++)
		sub(&a, b);
	print(a);
	sub(&a, a);
	print(a);
	sub(&a, z);
	print(a);
	add(&a, z);
	print(a);
	clear(&a);
	add(&a, b);
	print(a);
	clear(&a);
	clear(&b);
	clear(&z);
	return 0;
}
