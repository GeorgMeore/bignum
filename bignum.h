typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;

/* TODO: add a "shift" field to be able to store and work
 * with numbers like n << 1000 more efficiently */
typedef struct {
	uint len;
	uint cap;
	ulong *d;
	uchar neg;
} Number;

Number number(long n);
Number copy(Number n);
void   move(Number *dst, Number *src);
void   assign(Number *dst, Number src);
void   clear(Number *n);
ulong  bitlen(Number n);
void   negate(Number *n);
void   zero(Number *n);
int    iszero(Number n);
int    cmp(Number a, Number b);
void   add(Number *dst, Number src);
void   sub(Number *dst, Number src);
void   rshift(Number *n, uint bits);
void   lshift(Number *n, uint bits);
void   inc(Number *dst, ulong n);
void   dec(Number *dst, ulong n);
void   square(Number *n);
void   mul(Number *dst, Number src);
void   rem(Number *dst, Number src);
void   quo(Number *dst, Number src);
void   quorem(Number *dst, Number *rem, Number src);
void   read(Number *n, char *s);
void   print10(Number n);
void   print16(Number n);
