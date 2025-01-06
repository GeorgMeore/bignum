#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DEFAULTCAP 16

typedef struct {
	int len;
	int cap;
	unsigned long *d;
	int neg;
} Number;

Number number(unsigned long n)
{
	Number a;
	a.len = 1;
	a.cap = DEFAULTCAP;
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
	for (int i = 0; i < c.len; i++)
		c.d[i] = n.d[i];
	c.neg = n.neg;
	return c;
}

void destroy(Number n)
{
	free(n.d);
	n.d = 0;
}

void neg(Number *n)
{
	n->neg = !n->neg;
}

void zero(Number *n)
{
	n->neg = 0;
	for (int i = 0; i < n->len; i++)
		n->d[i] = 0;
	n->len = 1;
}

int iszero(Number n)
{
	return n.len == 1 && n.d[0] == 0;
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

static void extend(Number *n, int chunks)
{
	assert(chunks > 0);
	n->len += chunks;
	if (n->len <= n->cap)
		return;
	while (n->len > n->cap)
		n->cap *= 2;
	n->d = reallocarray(n->d, n->cap, sizeof(n->d[0]));
	for (int j = n->len - chunks; j < n->cap; j++)
		n->d[j] = 0;
}

static void absadd(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int carry = 0;
	for (int i = 0; i < src.len; i++) {
		dst->d[i] += src.d[i] + carry;
		carry = (carry && !(src.d[i] + carry)) || dst->d[i] < src.d[i] + carry;
	}
	for (int i = src.len; i < dst->len && carry; i++) {
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
	for (int i = 0; i < n->len; i++)
		n->d[i] = ~n->d[i];
	n->d[0] += 1;
}

void abssub(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int borrow = 0;
	for (int i = 0; i < src.len; i++) {
		dst->d[i] -= src.d[i] + borrow;
		borrow = (borrow && !(src.d[i] + borrow)) || dst->d[i] > ~(src.d[i] + borrow);
	}
	for (int i = src.len; i < dst->len && borrow; i++) {
		dst->d[i] -= borrow;
		borrow = !~dst->d[i];
	}
	if (borrow) {
		neg(dst);
		flip(dst);
	}
	shrink(dst);
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

void read(Number *n, char *s)
{
	zero(n);
	if (*s == '-') {
		n->neg = 1;
		s++;
	}
	int perchunk = (sizeof(n->d[0]) * 2);
	int len = strlen(s);
	int last = len / perchunk;
	if (last >= n->len)
		extend(n, last + 1 - n->len);
	for (int i = 0; i < len; i++) {
		char c = s[len-1-i];
		unsigned long digit;
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
	for (int i = n.len - 1; i >= 0; i--)
		printf("%lx", n.d[i]);
	printf("\n");
}

int main(int, char **)
{
	Number a = number(0);
	Number b = number(0x123450000ffff);
	Number z = number(0x0);
	read(&a, "123456789abcdefedcba9876543210");
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
	destroy(a);
	destroy(b);
	destroy(z);
	return 0;
}
