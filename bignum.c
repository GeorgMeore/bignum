#include <stdlib.h>
#include <stdio.h>

#define DEFAULTCAP 16

typedef struct {
	int len;
	int cap;
	unsigned long *d;
} Number;

Number number(unsigned long n)
{
	Number a;
	a.len = 1;
	a.cap = DEFAULTCAP;
	a.d = calloc(a.cap, sizeof(unsigned long));
	a.d[0] = n;
	for (int i = 1; i < a.cap; i++)
		a.d[i] = 0;
	return a;
}

Number copy(Number n)
{
	Number c;
	c.len = n.len;
	c.cap = n.cap;
	c.d = calloc(c.cap, sizeof(unsigned long));
	for (int i = 0; i < c.cap; i++)
		c.d[i] = n.d[i];
	return c;
}

void destroy(Number n)
{
	free(n.d);
	n.d = 0;
}

static void extend(Number *n, int chunks)
{
	n->len += chunks;
	if (n->len <= n->cap)
		return;
	while (n->len > n->cap)
		n->cap *= 2;
	n->d = reallocarray(n->d, n->cap, sizeof(unsigned long));
	for (int j = n->len - chunks; j < n->cap; j++)
		n->d[j] = 0;
}

int cmp(Number a, Number b)
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

void add(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int carry = 0;
	for (int i = 0; i < src.len; i++) {
		dst->d[i] += src.d[i] + carry;
		carry = dst->d[i] < src.d[i];
	}
	if (carry) {
		if (dst->len > src.len) {
			dst->d[src.len] += 1;
		} else {
			extend(dst, 1);
			dst->d[src.len] = 1;
		}
	}
}

static void shrink(Number *n)
{
	while (n->len > 1 && !n->d[n->len-1]) {
		n->len -= 1;
	}
}

// For now dst must be bigger
void sub(Number *dst, Number src)
{
	int borrow = 0;
	for (int i = 0; i < src.len; i++) {
		if (dst->d[i] < src.d[i] || (borrow && dst->d[i] == src.d[i])) {
			dst->d[i] = dst->d[i] + ~src.d[i] + 1 - borrow;
			borrow = 1;
		} else {
			dst->d[i] = dst->d[i] - src.d[i] - borrow;
			borrow = 0;
		}
	}
	if (borrow) {
		dst->d[src.len] -= 1;
	}
	shrink(dst);
}

Number sum(Number a, Number b)
{
	Number c = copy(a);
	add(&c, b);
	return c;
}

void print(Number n)
{
	for (int i = n.len - 1; i >= 0; i--)
		printf("%lx", n.d[i]);
	printf("\n");
}

int main(int, char **)
{
	Number a = number(0xFFFFFFFFFFFFFFFF);
	Number b = number(0xFFFFFFFFFFFFFFFF);
	print(a);
	for (int i = 0; i < 1000; i++)
		add(&a, b);
	print(a);
	for (int i = 0; i < 1000; i++)
		sub(&a, b);
	print(a);
	sub(&a, a);
	print(a);
	destroy(a);
	destroy(b);
	return 0;
}
