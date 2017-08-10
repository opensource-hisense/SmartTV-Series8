/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */

/*
 * Based off public domain RSA Public-Key Cryptography package by Philip J.
 * Erdelsky <pje@efgh.com>.
 * http://www.alumni.caltech.edu/~pje/
 */
#include <crypto/mpuint.h>

#include <linux/slab.h>
#include <linux/string.h>

struct mpuint
{
	uint8_t *value;  /* value is stored little-endian */
	unsigned length; /* bytes */
};

mpuint *mpuint_alloc(unsigned length)
{
	mpuint *x = (mpuint*)kmalloc(sizeof(mpuint), GFP_KERNEL);
	if (!x) return NULL;
	x->length = length;
	x->value = (uint8_t*)kmalloc(length * sizeof(uint8_t), GFP_KERNEL);
	if (!x->value)
	{
		kfree(x); x = NULL;
		return NULL;
	}
	memset(x->value, 0, length * sizeof(uint8_t));
	return x;
}

void mpuint_delete(mpuint *x)
{
	if (x)
	{
		kfree(x->value); x->value = NULL;
		kfree(x); x = NULL;
	}
}

mpuint *mpuint_copy(const mpuint *x)
{
	mpuint *y = mpuint_alloc(x->length);
	if (!y) return NULL;
	memcpy(y->value, x->value, x->length * sizeof(uint8_t));
	y->length = x->length;
	return y;
}

unsigned mpuint_length(const mpuint *x)
{
	return x->length;
}

mpuint *mpuint_resize(mpuint *x, unsigned length)
{
	unsigned i;
	if (length == x->length) return x;
	x->value = (uint8_t*)krealloc(x->value, length * sizeof(uint8_t), GFP_KERNEL);
	if (!x->value)
	{
		mpuint_delete(x); x = NULL;
		return NULL;
	}
	for (i = x->length; i < length; ++i) {
		x->value[i] = 0;
	}
	x->length = length;
	return x;
}

mpuint *mpuint_trim(mpuint *x)
{
	unsigned l = x->length;
	while (x->value[l - 1] == 0)
		--l;
	if (l == x->length) return x;
	x->value = (uint8_t*)krealloc(x->value, l * sizeof(uint8_t), GFP_KERNEL);
	if (!x->value)
	{
		mpuint_delete(x); x = NULL;
		return NULL;
	}
	x->length = l;
	return x;
}

const uint8_t *mpuint_value(const mpuint *x)
{
	return x->value;
}
	
int mpuint_divide(const mpuint *x, const mpuint *y, mpuint **pq, mpuint **pr)
{
	unsigned i = x->length;
	unsigned bit = 8;
	if (mpuint_iszero(y)) goto failure;
	mpuint_setint(*pr, 0);
	mpuint_setint(*pq, 0);
	while (i-- != 0)
	{
		bit = 8;
		while (bit-- != 0)
		{
			*pr = mpuint_shift(*pr, x->value[i] >> bit & 1);
			if (!*pr) goto failure;
			if (mpuint_le(y, *pr))
			{
				*pq = mpuint_shift(*pq, 1);
				if (!*pq) goto failure;
				*pr = mpuint_sub(*pr, y);
				if (!*pr) goto failure;
			}
			else
			{
				*pq = mpuint_shift(*pq, 0);
				if (!*pq) goto failure;
			}
		}
	}
	return 1;
failure:
	mpuint_delete(*pq); *pq = NULL;
	mpuint_delete(*pr); *pr = NULL;
	return 0;
}

int mpuint_iszero(const mpuint *x)
{
	unsigned i = 0;
	for (i = 0; i < x->length; ++i)
	{
		if (x->value[i] != 0)
			return 0;
	}
	return 1;
}

int mpuint_compare(const mpuint *x, const mpuint *y)
{
	unsigned i;
	if (x->length > y->length)
	{
		for (i = x->length - 1; i >= y->length; --i)
		{
			if (x->value[i] != 0)
				return 1;
			if (i == 0) break;
		}
	}
	else if (y->length > x->length)
	{
		for (i = y->length - 1; i >= x->length; --i)
		{
			if (y->value[i] != 0)
				return -1;
			if (i == 0) break;
		}
	}
	else
		i = x->length - 1;
	while (i >= 0)
	{
		if (x->value[i] > y->value[i])
			return 1;
		if (x->value[i] < y->value[i])
			return -1;
		if (i == 0) break;
		--i;
	}
    return 0;
}

int mpuint_compareint(const mpuint *x, uint8_t y)
{
	unsigned i;
	for (i = x->length - 1; i >= 1; --i)
	{
		if (x->value[i] != 0)
			return 1;
	}
	return x->value[0] > y ? 1 : x->value[0] < y ? -1 : 0;
}

int mpuint_eqint(const mpuint *x, uint8_t y)
{
	return mpuint_compareint(x, y) == 0;
}

int mpuint_eq(const mpuint *x, const mpuint *y)
{
	return mpuint_compare(x, y) == 0;
}

int mpuint_le(const mpuint *x, const mpuint *y)
{
	return mpuint_compare(x, y) <= 0;
}

void mpuint_setint(mpuint *x, uint8_t y)
{
	unsigned i;
	x->value[0] = y;
	for (i = 1; i < x->length; ++i)
		x->value[i] = 0;
}

mpuint *mpuint_setbytes(mpuint *x, const uint8_t *y, unsigned len)
{
	unsigned i;
	if (len > x->length)
	{
		x = mpuint_resize(x, len);
		if (!x) return NULL;
	}
    /* y is big-endian; x is little-endian */
	for (i = 0; i < len; ++i)
		x->value[i] = y[len - 1 - i];
	for (; i < x->length; ++i)
		x->value[i] = 0;
	return x;
}

unsigned mpuint_getbytes(const mpuint *x, uint8_t *y)
{
	unsigned i;
	/* x is little-endian; y is big-endian */
	for (i = 0; i < x->length; ++i)
		y[x->length - 1 - i] = x->value[i];
	return x->length;
}

mpuint *mpuint_set(mpuint *x, const mpuint *y)
{
	unsigned i;
	/* The original implementation would throw numeric overthrow if y was bigger
	 * than x. I see no reason not to just make this work. */
	if (y->length > x->length)
	{
		x = mpuint_resize(x, y->length);
		if (!x) return NULL;
	}
	for (i = 0; i < x->length && i < y->length; ++i)
		x->value[i] = y->value[i];
	for (; i < x->length; ++i)
		x->value[i] = 0;
	return x;
}

mpuint *mpuint_mul(mpuint *x, const mpuint *y)
{
	unsigned i, j, k;
	unsigned xlen = x->length;
	uint16_t product;
	uint8_t *multiplier = (uint8_t*)kmalloc(xlen * sizeof(uint8_t), GFP_KERNEL);
	for (i = 0; i < xlen; ++i)
	{
		multiplier[i] = x->value[i];
		x->value[i] = 0;
	}
	for (i = 0; i < xlen; ++i)
	{
		for (j = 0; j < y->length; ++j)
		{
			product = multiplier[i] * y->value[j];
			k = i + j;
			while (product != 0)
			{
				/* This originally threw numeric overflow if k >= x->length but
				 * I can make this work by doubling the allocation of x and then
				 * trimming later. */
				if (k >= x->length)
				{
					x = mpuint_resize(x, 2 * x->length);
					if (!x)
					{
						kfree(multiplier); multiplier = NULL;
						return NULL;
					}
				}
				product += x->value[k];
				x->value[k] = product;
				product >>= 8;
				k++;
			}
		}
	}
	x = mpuint_trim(x);
	kfree(multiplier); multiplier = NULL;
	return x;
}

mpuint *mpuint_addint(mpuint *x, uint8_t y)
{
	unsigned i;
	x->value[0] += y;
	if (x->value[0] < y)
	{
		for (i = 1; i < x->length; ++i)
		{
			if (++(x->value[i]) != 0)
				return x;
		}
		/* This originally threw numeric overflow if carry bits where left over.
		 * I can make this work by increasing the size by one to ensure the
		 * carry will always fit. */
		x = mpuint_resize(x, x->length + 1);
		if (!x) return NULL;
		++(x->value[x->length - 1]);
	}
	return x;
}

mpuint *mpuint_add(mpuint *x, const mpuint *y)
{
	unsigned i;
	unsigned long sum;
	unsigned long carry = 0;
	if (x->length < y->length)
	{
		x = mpuint_resize(x, y->length);
		if (!x) return NULL;
	}
	for (i = 0; i < x->length; ++i)
	{
		sum = x->value[i] + (i < y->length ? y->value[i] : 0) + carry;
		x->value[i] = sum;
		carry = sum >> 8;
	}
	/* This originally threw numeric overflow if carry bits were left over or x
	 * was smaller than y. I can make this work by increasing the size by one
	 * to ensure the carry will always fit. */
	if (carry != 0)
	{
		x = mpuint_resize(x, x->length + 1);
		if (!x) return NULL;
		x->value[x->length - 1] = carry;
	}
	return x;
}

mpuint *mpuint_subint(mpuint *x, uint8_t y)
{
	unsigned i;
	if (x->value[0] >= y)
	{
		x->value[0] -= y;
	}
	else
	{
		x->value[0] -= y;
		for (i = 1; i < x->length; ++i)
		{
			if (--(x->value[i]) != 0xFF)
				return x;
		}
		/* This originally threw numeric overflow if borrow bits were left over.
		 * I can't make this work because this means y is larger than x, and the
		 * result is a negative number. */
		mpuint_delete(x); x = NULL;
	}
	return x;
}

mpuint *mpuint_sub(mpuint *x, const mpuint *y)
{
	unsigned i;
	unsigned long borrow = 0;
	for (i = 0; i < x->length; ++i)
	{
		unsigned long subtrahend = (i < y->length ? y->value[i] : 0) + borrow;
		borrow = (unsigned long)(x->value[i]) < subtrahend;
		x->value[i] -= subtrahend;
	}
	/* This originally threw numeric overflow if borrow bits were left over or
	 * x was smaller than y. I can't make this work because this means y is
	 * larger than x, and the result is a negative number. */
	if (borrow != 0)
	{
		mpuint_delete(x); x = NULL;
		return NULL;
	}
	for (; i < y->length; ++i)
	{
		if (y->value[i] != 0)
		{
			mpuint_delete(x); x = NULL;
			return NULL;
		}
	}
	return x;
}

mpuint *mpuint_mod(mpuint *x, const mpuint *y)
{
	mpuint *quotient = NULL, *dividend = NULL;
	quotient = mpuint_alloc(x->length);
	if (!quotient) return NULL;
	dividend = mpuint_copy(x);
	if (!dividend)
	{
		mpuint_delete(quotient); quotient = NULL;
		return NULL;
	}
	mpuint_divide(dividend, y, &quotient, &x);
	mpuint_delete(dividend); dividend = NULL;
	mpuint_delete(quotient); quotient = NULL;
	return x;
}

mpuint *mpuint_shift(mpuint *x, unsigned bit)
{
	/* I think bit is actually supposed to be a carry bit. Then this makes a bit
	 * more sense: shift left by one, OR with the carry, figure out if the shift
	 * carries into the next larger value and repeat. */
	unsigned i;
	unsigned long shift;
	for (i = 0; i < x->length; ++i)
	{ 
		shift = x->value[i] << 1 | bit; 
		x->value[i] = shift;
		bit = shift >> 8;
	} 
	/* This originally threw numeric overflow if bits were left over. I can make
	 * this work by increasing the length by one. */
	if (bit != 0)
	{
		x = mpuint_resize(x, x->length + 1);
		if (!x) return NULL;
		x->value[x->length - 1] = bit;
	}
	return x;
}

mpuint *mpuint_power(const mpuint *b, const mpuint *e, const mpuint *m)
{
	int one = 1;
	unsigned i = e->length;
	unsigned bit;
	mpuint *n = NULL;
	mpuint *r = mpuint_alloc(2 * b->length + 1);
	if (!r) return NULL;
	mpuint_setint(r, 1);
	while (i-- != 0)
	{
		bit = 1 << 7;
		do
		{
			if (!one)
			{
				n = mpuint_copy(r);
				if (!n) goto failure;
				r = mpuint_mul(r, n);
				if (!r) goto failure;
				mpuint_delete(n); n = NULL;
				r = mpuint_mod(r, m);
				if (!r) goto failure;
			}
			if (e->value[i] & bit)
			{
				r = mpuint_mul(r, b);
				if (!r) goto failure;
				r = mpuint_mod(r, m);
				if (!r) goto failure;
				one = 0;
			}
			bit >>= 1;
		} while (bit != 0);
	}
	return r;
failure:
	mpuint_delete(r); r = NULL;
	mpuint_delete(n); n = NULL;
	return NULL;
}