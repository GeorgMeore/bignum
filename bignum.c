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

#define CHUNKBITS (sizeof(ulong)*8)

Number number(ulong n)
{
	Number a = {};
	a.len = 1;
	a.cap = 16; /* the default initial capacity */
	a.d = calloc(a.cap, sizeof(a.d[0]));
	a.d[0] = n; /* all others are zeroed by calloc */
	return a;
}

Number copy(Number n)
{
	Number c = {};
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

void negate(Number *n)
{
	n->neg = !n->neg;
}

void zero(Number *n)
{
	if (n->len) {
		for (uint i = 0; i < n->len; i++)
			n->d[i] = 0;
		n->len = 1;
	}
	n->neg = 0;
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
		negate(dst);
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
	negate(&src);
	add(dst, src);
}

void rshift(Number *n, uint bits)
{
	if (iszero(*n))
		return;
	uint drop = bits / CHUNKBITS;
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
	uint shift = bits % CHUNKBITS;
	if (shift) {
		ulong mask = (1UL << shift) - 1;
		n->d[0] >>= shift;
		for (uint i = 0; i < n->len-drop-1; i++) {
			n->d[i] |= (n->d[i+1] & mask) << (CHUNKBITS - shift);
			n->d[i+1] >>= shift;
		}
	}
	shrink(n);
}

void lshift(Number *n, uint bits)
{
	if (iszero(*n))
		return;
	uint move = bits / CHUNKBITS;
	if (move) {
		extend(n, move);
		for (uint i = 0; i < n->len-move; i++) {
			n->d[n->len-1-i] = n->d[n->len-1-i - move];
			n->d[n->len-1-i - move] = 0;
		}
	}
	uint shift = bits % CHUNKBITS;
	if (shift) {
		ulong mask = ((1UL << shift) - 1) << (CHUNKBITS - shift);
		if (n->d[n->len-1] & mask)
			extend(n, 1);
		else
			n->d[n->len-1] <<= shift;
		for (uint i = 0; i < n->len-move-1; i++) {
			n->d[n->len-i-1] |= (n->d[n->len-i-2] & mask) >> (CHUNKBITS - shift);
			n->d[n->len-i-2] <<= shift;
		}
	}
}

ulong length(Number n)
{
	if (n.len) {
		for (uint i = CHUNKBITS; i; i--) {
			if (n.d[n.len-1] & (1UL << (i-1)))
				return (ulong)(n.len - 1)*CHUNKBITS + i;
		}
	}
	return 0;
}

void inc(Number *dst, ulong n)
{
	Number c = {1, 1, (ulong[]){n}, 0};
	add(dst, c);
}

void dec(Number *dst, ulong n)
{
	Number c = {1, 1, (ulong[]){n}, 0};
	sub(dst, c);
}

void mul(Number *dst, Number src)
{
	if (iszero(src)) {
		zero(dst);
		return;
	}
	dst->neg = dst->neg ^ src.neg;
	Number tmp = copy(*dst);
	uint srccopy = dst->d == src.d;
	if (srccopy)
		src = copy(src);
	zero(dst);
	for (ulong i = 0; i < length(src); i++) {
		if (src.d[i/CHUNKBITS] & (1UL << i%CHUNKBITS))
			absadd(dst, tmp);
		lshift(&tmp, 1);
	}
	clear(&tmp);
	if (srccopy)
		clear(&src);
}

void move(Number *dst, Number *src)
{
	if (dst->d == src->d)
		return;
	free(dst->d);
	*dst = *src;
	src->len = 0;
	src->cap = 0;
	src->d = 0;
}

void divrem(Number *dst, Number *rem, Number src)
{
	if (iszero(src))
		return; /* should we crash? */
	ulong dstlen = length(*dst);
	ulong srclen = length(src);
	if (dstlen < srclen) {
		if (rem)
			move(rem, dst);
		else
			zero(dst);
		return;
	}
	Number d = copy(src);
	Number r = copy(*dst);
	dst->neg = r.neg = dst->neg ^ src.neg;
	lshift(&d, dstlen - srclen);
	for (uint i = 0; i < dst->len; i++)
		dst->d[i] = 0;
	for (ulong i = dstlen - 1; i >= srclen - 1; i--) {
		if (abscmp(r, d) >= 0) {
			abssub(&r, d);
			dst->d[i/CHUNKBITS] |= 1UL << i%CHUNKBITS;
		}
		rshift(&d, 1);
	}
	rshift(dst, srclen - 1);
	if (rem)
		move(rem, &r);
	else
		clear(&r);
	clear(&d);
}

void read(Number *n, char *s)
{
	zero(n);
	if (*s == '-') {
		n->neg = 1;
		s++;
	}
	uint charbits = CHUNKBITS/4;
	uint slen = strlen(s);
	uint last = (slen + charbits - 1) / charbits;
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
		int chunk = i / charbits;
		int shift = i % charbits;
		n->d[chunk] += digit << shift*4;
	}
	shrink(n);
}

void print10(Number n)
{
	if (iszero(n)) {
		printf("0\n");
		return;
	}
	if (n.neg)
		printf("-");
	uint len10 = length(n) / 3;
	char *digits = malloc(len10+1);
	char *curr = digits + len10;
	Number d = copy(n);
	Number r = number(0);
	Number ten = {1, 1, (ulong[]){10}, 0};
	for (*curr = '\0', curr--; !iszero(d); curr--) {
		divrem(&d, &r, ten);
		*curr = '0' + r.d[0];
	}
	printf("%s\n", curr);
	free(digits);
	clear(&d);
	clear(&r);
}

void print16(Number n)
{
	if (iszero(n)) {
		printf("0x0\n");
		return;
	}
	if (n.neg)
		printf("-");
	printf("0x%lx", n.d[n.len-1]);
	for (uint i = 1; i < n.len; i++)
		printf("%016lx", n.d[n.len-1-i]);
	printf("\n");
}

void fact(ulong x)
{
	Number acc = number(1);
	while (x > 1) {
		Number t = {1, 1, (ulong[]){x}, 0};
		mul(&acc, t);
		x -= 1;
	}
	print10(acc);
	clear(&acc);
}

/* TODO: tests */
int main(void)
{
	Number a = number(4);
	Number b = number(55);
	Number c = number(0);
	fact(102);
	read(&a, "123456789abcdefedcba9876543210");
	print10(a);
	read(&b, "912374198327491832749817348971298374981273498719283749817234987129384791234981237498123749875192357192381423");
	divrem(&b, &c, a);
	print16(b);
	print16(c);
	print16(a);
	clear(&a);
	clear(&b);
	clear(&c);
	return 0;
}
