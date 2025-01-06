#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define SWAP(x, y) ({typeof(x) tmp = y; y = x; x = tmp;})

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
	a.d = calloc(a.cap, sizeof(unsigned long));
	a.d[0] = n; /* all others are zeroed by calloc */
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

void neg(Number *n)
{
	n->neg = !n->neg;
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
	n->d = reallocarray(n->d, n->cap, sizeof(unsigned long));
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
	while (n->len > 1 && !n->d[n->len-1])
		n->len -= 1;
}

void abssub(Number *dst, Number src)
{
	Number m = *dst, s = src;
	if (abscmp(m, s) == -1) {
		neg(dst);
		SWAP(m, s);
	}
	if (dst->len < m.len)
		extend(dst, m.len - dst->len);
	int borrow = 0;
	for (int i = 0; i < s.len; i++) {
		if (m.d[i] < s.d[i] || (borrow && m.d[i] == s.d[i])) {
			dst->d[i] = m.d[i] + ~s.d[i] + 1 - borrow;
			borrow = 1;
		} else {
			dst->d[i] = m.d[i] - s.d[i] - borrow;
			borrow = 0;
		}
	}
	for (int i = s.len; i < m.len; i++)
		dst->d[i] = m.d[i];
	if (borrow)
		dst->d[s.len] -= 1;
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

Number sum(Number a, Number b)
{
	Number c = copy(a);
	add(&c, b);
	return c;
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
	Number a = number(0xFFFFFFFFFFFFFFFF);
	Number b = number(0xFFFFFFFFFFFFFFFF);
	Number z = number(0x0);
	print(a);
	neg(&a);
	print(a);
	for (int i = 0; i < 1000; i++)
		add(&a, b);
	print(a);
	for (int i = 0; i < 1000; i++)
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
