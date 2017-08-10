/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */

/* 
 * Based off Crypto++ SHA-1 by Steve Reid.
 * http://sourceforge.net/projects/cryptopp/
 */
#include <crypto/mpusha.h>

#include <linux/slab.h>
#include <linux/string.h>

#define SHA1_BLOCKSIZE 64
#define PADDING_BYTES 56

/* define this if running on a big-endian CPU */
#if !defined(IS_LITTLE_ENDIAN) && (defined(__BIG_ENDIAN__) || defined(__sparc) || defined(__sparc__) || defined(__hppa__) || defined(__MIPSEB__) || defined(__ARMEB__) || (defined(__MWERKS__) && !defined(__INTEL__)))
#	define IS_BIG_ENDIAN
#endif

/* define this if running on a little-endian CPU */
#ifndef IS_BIG_ENDIAN
#	define IS_LITTLE_ENDIAN
#endif

struct sha1_state
{
	uint32_t hash[5];
	uint8_t pending[SHA1_BLOCKSIZE];
	uint32_t plen; /* pending count in bytes */
	uint64_t tlen; /* total message length in bits */
};

static inline uint32_t rotlFixed(uint32_t x, unsigned int y)
{
	return (uint32_t)((x<<y) | (x>>(sizeof(uint32_t)*8-y)));
}

static inline uint32_t rotrFixed(uint32_t x, unsigned int y)
{
	return (uint32_t)((x>>y) | (x<<(sizeof(uint32_t)*8-y)));
}

static inline uint32_t byteReverse(uint32_t value)
{
#if defined(__GNUC__) && defined(CRYPTOPP_X86_ASM_AVAILABLE)
	__asm__ ("bswap %0" : "=r" (value) : "0" (value));
	return value;
#else
	/* 6 instructions with rotate instruction, 8 without */
	value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
	return rotlFixed(value, 16U);
#endif
}

#ifdef IS_LITTLE_ENDIAN
#define blk0(i) (W[i] = byteReverse(data[i]))
#else
#define blk0(i) (W[i] = data[i])
#endif
#define blk1(i) (W[i&15] = rotlFixed(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rotlFixed(v,5);w=rotlFixed(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rotlFixed(v,5);w=rotlFixed(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rotlFixed(v,5);w=rotlFixed(w,30);

struct sha1_state *sha1_init()
{
	struct sha1_state *state = (struct sha1_state*)kmalloc(sizeof(struct sha1_state), GFP_KERNEL);
	if (!state) return NULL;
	state->hash[0] = 0x67452301L;
	state->hash[1] = 0xEFCDAB89L;
    state->hash[2] = 0x98BADCFEL;
	state->hash[3] = 0x10325476L;
	state->hash[4] = 0xC3D2E1F0L;
	memset(state->pending, 0, SHA1_BLOCKSIZE);
	state->plen = 0;
	state->tlen = 0;
	return state;
}

void sha1_free(struct sha1_state *state)
{
	kfree(state); state = NULL;
}

/**
 * Feed 512 bits into the SHA-1.
 *
 * @param[in|out] state SHA-1 state.
 * @param[in] data 64-bytes (512-bits) of data.
 */
void sha1_transform(struct sha1_state *state, const uint32_t *data)
{
	uint32_t W[16];
    /* Copy state to working vars */
    uint32_t a = state->hash[0];
    uint32_t b = state->hash[1];
    uint32_t c = state->hash[2];
    uint32_t d = state->hash[3];
    uint32_t e = state->hash[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into state. */
    state->hash[0] += a;
    state->hash[1] += b;
    state->hash[2] += c;
    state->hash[3] += d;
    state->hash[4] += e;
}

void sha1_update(struct sha1_state *state, const uint8_t *data, uint32_t len)
{
	unsigned offset = 0;
	unsigned copylen = 0;
	/* Append to pending data if there is any pending data. */
	if (state->plen > 0 && len > 0)
	{
		copylen = SHA1_BLOCKSIZE - state->plen;
		if (copylen > len) copylen = len;
		memcpy(state->pending + state->plen, data, copylen);
		state->plen += copylen;
		offset += copylen;
	}
	/* Transform pending data if it is full. */
	if (state->plen == SHA1_BLOCKSIZE)
	{
		sha1_transform(state, (const uint32_t*)state->pending);
		state->plen = 0;
	}
	/* Transform remaining data if we have enough blocks. */
	while (len - offset >= SHA1_BLOCKSIZE)
	{
		sha1_transform(state, (const uint32_t*)(data + offset));
		offset += SHA1_BLOCKSIZE;
	}
	/* Store remaining data for later. */
	if (len - offset > 0)
	{
		copylen = len - offset;
		memcpy(state->pending, data + offset, copylen);
		state->plen = copylen;
	}
	state->tlen += len * 8;
}

uint8_t *sha1_digest(struct sha1_state *state, const uint8_t *data, uint32_t len)
{
	sha1_update(state, data, len);
	return sha1_final(state);
}

uint8_t *sha1_final(struct sha1_state *state)
{
	unsigned i = 0;
	uint8_t *digest = NULL;
	uint64_t msglen = state->tlen;
	/* Append a '1' bit, padded out to 56 bytes (448 bits) of pending data. */
	state->pending[state->plen] = 0x80;
	++state->plen;
	if (state->plen < PADDING_BYTES)
	{
		memset(state->pending + state->plen, 0, PADDING_BYTES - state->plen);
		state->plen = PADDING_BYTES;
	}
	else if (state->plen > PADDING_BYTES)
	{
		memset(state->pending + state->plen, 0, SHA1_BLOCKSIZE - state->plen);
		sha1_transform(state, (const uint32_t*)state->pending);
		memset(state->pending, 0, PADDING_BYTES);
		state->plen = PADDING_BYTES;
	}
	/* Append the 8 byte (64 bit) message length in bits. */
	state->pending[state->plen++] = (msglen >> 56) & 0xFF;
	state->pending[state->plen++] = (msglen >> 48) & 0xFF;
	state->pending[state->plen++] = (msglen >> 40) & 0xFF;
	state->pending[state->plen++] = (msglen >> 32) & 0xFF;
	state->pending[state->plen++] = (msglen >> 24) & 0xFF;
	state->pending[state->plen++] = (msglen >> 16) & 0xFF;
	state->pending[state->plen++] = (msglen >> 8) & 0xFF;
	state->pending[state->plen++] = (msglen >> 0) & 0xFF;
	sha1_transform(state, (const uint32_t*)state->pending);
	/* Compute the final digest. */
	digest = kmalloc(20 * sizeof(uint8_t), GFP_KERNEL);
	for (i = 0; digest && i < 5; ++i)
	{
		digest[i * 4 + 0] = (state->hash[i] >> 24) & 0xFF;
		digest[i * 4 + 1] = (state->hash[i] >> 16) & 0xFF;
		digest[i * 4 + 2] = (state->hash[i] >> 8) & 0xFF;
		digest[i * 4 + 3] = (state->hash[i] >> 0) & 0xFF;
	}
	return digest;
}

#define SHA256_BLOCKSIZE 64

struct sha256_state
{
	uint32_t hash[8];
	uint8_t pending[SHA256_BLOCKSIZE];
	uint32_t plen; /* pending count in bytes */
	uint64_t tlen; /* total message length in bits */
};

const uint32_t SHA256_K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) (y^((x^y)&(y^z)))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+SHA256_K[i+j]+(j?blk2(i):blk0(i));\
	d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))

/* SHA-256 operations */
#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))

struct sha256_state *sha256_init()
{
	struct sha256_state *state = (struct sha256_state*)kmalloc(sizeof(struct sha256_state), GFP_KERNEL);
	if (!state) return NULL;
	state->hash[0] = 0x6a09e667;
	state->hash[1] = 0xbb67ae85;
	state->hash[2] = 0x3c6ef372;
	state->hash[3] = 0xa54ff53a;
	state->hash[4] = 0x510e527f;
	state->hash[5] = 0x9b05688c;
	state->hash[6] = 0x1f83d9ab;
	state->hash[7] = 0x5be0cd19;
	memset(state->pending, 0, SHA256_BLOCKSIZE);
	state->plen = 0;
	state->tlen = 0;
	return state;
}

void sha256_free(struct sha256_state *state)
{
	kfree(state); state = NULL;
}

/**
 * Feed 512 bits into the SHA-256.
 *
 * @param[in|out] state SHA-256 state.
 * @param[in] data 64-bytes (512 bits) of data.
 */
void sha256_transform(struct sha256_state *state, const uint32_t *data)
{
	unsigned int j;
	uint32_t W[16];
	/* Copy state to working vars. */
	uint32_t T[8];
	memcpy(T, state->hash, sizeof(T));
	/* 64 operations, partially loop unrolled. */
	for (j = 0; j < 64; j += 16)
	{
		R( 0); R( 1); R( 2); R( 3);
		R( 4); R( 5); R( 6); R( 7);
		R( 8); R( 9); R(10); R(11);
		R(12); R(13); R(14); R(15);
	}
	/* Add the working vars back into state. */
	state->hash[0] += a(0);
	state->hash[1] += b(0);
	state->hash[2] += c(0);
	state->hash[3] += d(0);
	state->hash[4] += e(0);
	state->hash[5] += f(0);
	state->hash[6] += g(0);
	state->hash[7] += h(0);
}

void sha256_update(struct sha256_state *state, const uint8_t *data, uint32_t len)
{
	unsigned offset = 0;
	unsigned copylen = 0;
	/* Append to pending data if there is any pending data. */
	if (state->plen > 0 && len > 0)
	{
		copylen = SHA256_BLOCKSIZE - state->plen;
		if (copylen > len) copylen = len;
		memcpy(state->pending + state->plen, data, copylen);
		state->plen += copylen;
		offset += copylen;
	}
	/* Transform pending data if it is full. */
	if (state->plen == SHA256_BLOCKSIZE)
	{
		sha256_transform(state, (const uint32_t*)state->pending);
		state->plen = 0;
	}
	/* Transform remaining data if we have enough blocks. */
	while (len - offset >= SHA256_BLOCKSIZE)
	{
		sha256_transform(state, (const uint32_t*)(data + offset));
		offset += SHA256_BLOCKSIZE;
	}
	/* Store remaining data for later. */
	if (len - offset > 0)
	{
		copylen = len - offset;
		memcpy(state->pending, data + offset, copylen);
		state->plen = copylen;
	}
	state->tlen += len * 8;
}

uint8_t *sha256_digest(struct sha256_state *state, const uint8_t *data, uint32_t len)
{
	sha256_update(state, data, len);
	return sha256_final(state);
}

uint8_t *sha256_final(struct sha256_state *state)
{
	unsigned i = 0;
	uint8_t *digest = NULL;
	uint64_t msglen = state->tlen;
	/* Append a '1' bit, padded out to 56 bytes (448 bits) of pending data. */
	state->pending[state->plen] = 0x80;
	++state->plen;
	if (state->plen < PADDING_BYTES)
	{
		memset(state->pending + state->plen, 0, PADDING_BYTES - state->plen);
		state->plen = PADDING_BYTES;
	}
	else if (state->plen > PADDING_BYTES)
	{
		memset(state->pending + state->plen, 0, SHA256_BLOCKSIZE - state->plen);
		sha256_transform(state, (const uint32_t*)state->pending);
		memset(state->pending, 0, PADDING_BYTES);
		state->plen = PADDING_BYTES;
	}
	/* Append the 8 byte (64 bit) message length in bits. */
	state->pending[state->plen++] = (msglen >> 56) & 0xFF;
	state->pending[state->plen++] = (msglen >> 48) & 0xFF;
	state->pending[state->plen++] = (msglen >> 40) & 0xFF;
	state->pending[state->plen++] = (msglen >> 32) & 0xFF;
	state->pending[state->plen++] = (msglen >> 24) & 0xFF;
	state->pending[state->plen++] = (msglen >> 16) & 0xFF;
	state->pending[state->plen++] = (msglen >> 8) & 0xFF;
	state->pending[state->plen++] = (msglen >> 0) & 0xFF;
	sha256_transform(state, (const uint32_t*)state->pending);
	/* Compute the final digest. */
	digest = kmalloc(32 * sizeof(uint8_t), GFP_KERNEL);
	for (i = 0; digest && i < 8; ++i)
	{
		digest[i * 4 + 0] = (state->hash[i] >> 24) & 0xFF;
		digest[i * 4 + 1] = (state->hash[i] >> 16) & 0xFF;
		digest[i * 4 + 2] = (state->hash[i] >> 8) & 0xFF;
		digest[i * 4 + 3] = (state->hash[i] >> 0) & 0xFF;
	}
	return digest;
}

#undef S0
#undef S1
#undef s0
#undef s1
#undef R