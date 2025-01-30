#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "bignum.h"


#define CHUNKBITS (sizeof(ulong)*8)

ulong bitlen(Number n)
{
	if (n.len) {
		for (uint i = CHUNKBITS; i > 0; i--) {
			if (n.d[n.len-1] & (1UL << (i-1)))
				return (ulong)(n.len - 1)*CHUNKBITS + i;
		}
	}
	return 0;
}

Number number(long n)
{
	Number a = {};
	a.len = 1;
	a.cap = 16; /* the default initial capacity */
	a.d = calloc(a.cap, sizeof(a.d[0]));
	if (n < 0) {
		a.neg = 1;
		n = -n;
	}
	a.d[0] = n; /* all others are zeroed by calloc */
	return a;
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

static void shrink(Number *n)
{
	while (n->len > 1 && !n->d[n->len-1])
		n->len -= 1;
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

void move(Number *dst, Number *src)
{
	if (dst->d == src->d)
		return;
	free(dst->d);
	*dst = *src;
	*src = (Number){};
}

void assign(Number *dst, Number src)
{
	if (dst->d == src.d)
		return;
	if (dst->len < src.len)
		extend(dst, dst->len - src.len);
	for (uint i = 0; i < src.len; i++)
		dst->d[i] = src.d[i];
	for (uint i = src.len; i < dst->len; i++)
		dst->d[i] = 0;
	dst->len = src.len;
}

void clear(Number *n)
{
	free(n->d);
	*n = (Number){};
}

void negate(Number *n)
{
	n->neg = !n->neg;
}

void zero(Number *n)
{
	if (!n->len)
		return; /* We treat cleared numbers as zero */
	for (uint i = 0; i < n->len; i++)
		n->d[i] = 0;
	n->len = 1;
}

int iszero(Number n)
{
	return !n.len || (n.len == 1 && n.d[0] == 0);
}

static void absadd(Number *dst, Number src)
{
	if (dst->len < src.len)
		extend(dst, src.len - dst->len);
	int carry = 0;
	for (uint i = 0; i < src.len; i++) {
		ulong diff = src.d[i] + carry;
		dst->d[i] += diff;
		carry = (carry && !diff) || dst->d[i] < diff;
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

static void abssub(Number *dst, Number src, int gt)
{
	ulong *dgt, *dlt;
	if (gt) {
		dgt = dst->d, dlt = src.d;
	} else {
		extend(dst, src.len - dst->len);
		dgt = src.d, dlt = dst->d;
	}
	int borrow = 0;
	for (uint i = 0; i < src.len; i++) {
		ulong diff = dlt[i] + borrow;
		dst->d[i] = dgt[i] - diff;
		borrow = (borrow && !diff) || dst->d[i] > ~diff;
	}
	for (uint i = src.len; i < dst->len && borrow; i++) {
		dst->d[i] -= borrow;
		borrow = !~dst->d[i];
	}
	assert(!borrow);
	shrink(dst);
}

static int abscmp(Number a, Number b)
{
	if (a.len > b.len)
		return 1;
	if (a.len < b.len)
		return -1;
	for (long i = a.len - 1; i >= 0; i--) {
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
	if (dst->neg == src.neg) {
		absadd(dst, src);
	} else if (abscmp(*dst, src) > 0) {
		abssub(dst, src, 1);
	} else {
		abssub(dst, src, 0);
		negate(dst);
	}
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
		for (uint i = 0; i < n->len-drop; i++)
			n->d[i] = n->d[i+drop];
		for (uint i = n->len-drop; i < n->len; i++)
			n->d[i] = 0;
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
		for (uint i = n->len; i >= move; i--)
			n->d[i] = n->d[i-move];
		for (uint i = 0; i < move; i++)
			n->d[i] = 0;
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

static void ulmul(ulong x, ulong y, ulong lu[2])
{
	ulong a = x & 0xFFFFFFFF, b = x >> 32;
	ulong c = y & 0xFFFFFFFF, d = y >> 32;
	ulong t = a*d + c*b;
	ulong of1 = t < a*d;
	lu[0] = a*c + (t << 32);
	ulong of2 = lu[0] < a*c;
	lu[1] = b*d + (t >> 32) + of2 + (of1 << 32);
}

void square(Number *n)
{
	if (iszero(*n))
		return;
	uint l = n->len;
	extend(n, n->len);
	Number tmp = {2, 2, (ulong[]){0, 0}, 0};
	for (long i = n->len-2; i >= 0; i--) {
		long j0 = i < l ? i : l-1;
		for (long j = j0; j >= (i + 1)/2; j--) {
			ulmul(n->d[j], n->d[i-j], tmp.d);
			Number s = {n->len-i, n->len-i, &n->d[i], 0};
			if (j == j0)
				s.d[0] = 0; /* without that we get n**2 + n */
			absadd(&s, tmp);
			/* NOTE: We could do a shift by 1, but the edge cases are
			 * kinda tricky to handle, sooo... */
			if (j != i-j)
				absadd(&s, tmp);
		}
	}
	shrink(n);
}

void mul(Number *dst, Number src)
{
	if (dst->d == src.d)
		return square(dst);
	if (iszero(src))
		return zero(dst);
	dst->neg = dst->neg ^ src.neg;
	Number tmp = copy(*dst);
	zero(dst);
	for (ulong i = 0; i < bitlen(src); i++) {
		if (src.d[i/CHUNKBITS] & (1UL << i%CHUNKBITS))
			absadd(dst, tmp);
		lshift(&tmp, 1);
	}
	clear(&tmp);
}

void rem(Number *dst, Number src)
{
	assert(!iszero(src));
	dst->neg ^= src.neg;
	ulong dstlen = bitlen(*dst);
	ulong srclen = bitlen(src);
	if (dstlen > srclen)
		lshift(&src, dstlen - srclen);
	for (ulong i = dstlen; i >= srclen; i--) {
		if (abscmp(*dst, src) >= 0)
			abssub(dst, src, 1);
		if (i > srclen)
			rshift(&src, 1);
	}
}

void quo(Number *dst, Number src)
{
	assert(!iszero(src));
	dst->neg ^= src.neg;
	ulong dstlen = bitlen(*dst);
	ulong srclen = bitlen(src);
	if (dstlen > srclen)
		lshift(&src, dstlen - srclen);
	extend(dst, 1); /* dst = remainder, chunk, quotient */
	uint l = dst->len;
	dst->len -= 1;
	for (ulong i = dstlen - 1; i >= srclen - 1; i--) {
		if (abscmp(*dst, src) >= 0) {
			abssub(dst, src, 1);
			dst->d[i/CHUNKBITS + 1] |= 1UL << i%CHUNKBITS;
		}
		if (i > srclen - 1)
			rshift(&src, 1);
	}
	dst->len = l;
	rshift(dst, srclen - 1 + CHUNKBITS);
}

/* TODO: do less copying */
void quorem(Number *dst, Number *rem, Number src)
{
	if (!rem)
		return quo(dst, src);
	assert (!iszero(src));
	ulong dstlen = bitlen(*dst);
	ulong srclen = bitlen(src);
	if (dstlen < srclen)
		return move(rem, dst);
	Number d = copy(src);
	Number r = copy(*dst);
	dst->neg = r.neg = dst->neg ^ src.neg;
	for (uint i = 0; i < dst->len; i++)
		dst->d[i] = 0;
	lshift(&d, dstlen - srclen);
	for (ulong i = dstlen - srclen; ~i; i--) {
		if (abscmp(r, d) >= 0) {
			abssub(&r, d, 1);
			dst->d[i/CHUNKBITS] |= 1UL << i%CHUNKBITS;
		}
		rshift(&d, 1);
	}
	shrink(dst);
	move(rem, &r);
	clear(&d);
}

void read(Number *n, char *s)
{
	zero(n);
	if (*s == '-') {
		n->neg = 1;
		s++;
	} else {
		n->neg = 0;
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
		uint chunk = i / charbits;
		uint shift = i % charbits;
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
	uint len10 = (bitlen(n) + 2) / 3; /* round up */
	char *digits = malloc(len10+1);
	char *curr = digits + len10;
	Number d = copy(n);
	Number r = number(0);
	Number ten = {1, 1, (ulong[]){10}, 0};
	for (*curr = '\0'; !iszero(d); curr--) {
		quorem(&d, &r, ten);
		curr[-1] = '0' + r.d[0];
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
