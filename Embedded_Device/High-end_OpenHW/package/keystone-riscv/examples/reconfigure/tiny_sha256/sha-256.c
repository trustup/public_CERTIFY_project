#include "sha-256.h"

#define TOTAL_LEN_LEN 8

/*
 * Comments from pseudo-code at https://en.wikipedia.org/wiki/SHA-2 are reproduced here.
 * When useful for clarification, portions of the pseudo-code are reproduced here too.
 */

/*
 * @brief Rotate a 32-bit value by a number of bits to the right.
 * @param value The value to be rotated.
 * @param count The number of bits to rotate by.
 * @return The rotated value.
 */
static inline uint32_t right_rot(uint32_t value, unsigned int count)
{
	/*
	 * Defined behaviour in standard C for all count where 0 < count < 32, which is what we need here.
	 */
	return value >> count | value << (32 - count);
}

/*
 * @brief Update a hash value under calculation with a new chunk of data.
 * @param h Pointer to the first hash item, of a total of eight.
 * @param p Pointer to the chunk data, which has a standard length.
 *
 * @note This is the SHA-256 work horse.
 */
static inline void consume_chunk(uint32_t *h, const uint8_t *p)
{
	uint32_t w[64];
	uint32_t a, b, c, d, e, f, g, h0;
	unsigned int i;
	static const uint32_t k[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	// Prepare message schedule w[0..63]
	for (i = 0; i < 16; ++i) {
		w[i] = ((uint32_t)p[4 * i] << 24) |
			   ((uint32_t)p[4 * i + 1] << 16) |
			   ((uint32_t)p[4 * i + 2] << 8) |
			   ((uint32_t)p[4 * i + 3]);
	}
	for (i = 16; i < 64; ++i) {
		uint32_t s0 = right_rot(w[i - 15], 7) ^ right_rot(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = right_rot(w[i - 2], 17) ^ right_rot(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	// Initialize working variables
	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
	e = h[4];
	f = h[5];
	g = h[6];
	h0 = h[7];

	// Main compression loop
	for (i = 0; i < 64; ++i) {
		uint32_t S1 = right_rot(e, 6) ^ right_rot(e, 11) ^ right_rot(e, 25);
		uint32_t ch = (e & f) ^ ((~e) & g);
		uint32_t temp1 = h0 + S1 + ch + k[i] + w[i];
		uint32_t S0 = right_rot(a, 2) ^ right_rot(a, 13) ^ right_rot(a, 22);
		uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		uint32_t temp2 = S0 + maj;

		h0 = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}

	// Add the compressed chunk to the current hash value
	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
	h[5] += f;
	h[6] += g;
	h[7] += h0;
}

/*
 * Public functions. See header file for documentation.
 */

void sha_256_init(struct Sha_256 *sha_256, uint8_t hash[SIZE_OF_SHA_256_HASH])
{
	sha_256->hash = hash;
	sha_256->chunk_pos = sha_256->chunk;
	sha_256->space_left = SIZE_OF_SHA_256_CHUNK;
	sha_256->total_len = 0;
	/*
	 * Initialize hash values (first 32 bits of the fractional parts of the square roots of the first 8 primes
	 * 2..19):
	 */
	sha_256->h[0] = 0x6a09e667;
	sha_256->h[1] = 0xbb67ae85;
	sha_256->h[2] = 0x3c6ef372;
	sha_256->h[3] = 0xa54ff53a;
	sha_256->h[4] = 0x510e527f;
	sha_256->h[5] = 0x9b05688c;
	sha_256->h[6] = 0x1f83d9ab;
	sha_256->h[7] = 0x5be0cd19;
}

void sha_256_write(struct Sha_256 *sha_256, const void *data, size_t len)
{
	sha_256->total_len += len;

	const uint8_t *p = (const uint8_t *)data;

	while (len > 0) {
		if (sha_256->space_left == SIZE_OF_SHA_256_CHUNK && len >= SIZE_OF_SHA_256_CHUNK) {
			consume_chunk(sha_256->h, p);
			len -= SIZE_OF_SHA_256_CHUNK;
			p += SIZE_OF_SHA_256_CHUNK;
			continue;
		}
		const size_t consumed_len = len < sha_256->space_left ? len : sha_256->space_left;
		memcpy(sha_256->chunk_pos, p, consumed_len);
		sha_256->space_left -= consumed_len;
		len -= consumed_len;
		p += consumed_len;
		uint8_t *next_chunk_pos = sha_256->chunk_pos + consumed_len;
		uint8_t offset = (uint8_t*)(next_chunk_pos) - (uint8_t*)(sha_256->chunk);
		if (next_chunk_pos < sha_256->chunk || next_chunk_pos > sha_256->chunk + SIZE_OF_SHA_256_CHUNK) {
		}
		if (sha_256->space_left == 0) {
			consume_chunk(sha_256->h, sha_256->chunk);
			sha_256->chunk_pos = sha_256->chunk;
			sha_256->space_left = SIZE_OF_SHA_256_CHUNK;
		} else {
			sha_256->chunk_pos = next_chunk_pos;
		}
	}
}

uint8_t *sha_256_close(struct Sha_256 *sha_256)
{
	uint64_t total_bits = sha_256->total_len * 8;
	size_t used = (size_t)(sha_256->chunk_pos - sha_256->chunk);
	size_t pad_len = (used < 56) ? (56 - used) : (56 + 64 - used);
	uint8_t pad[64] = {0};
	pad[0] = 0x80;
	sha_256_write(sha_256, pad, pad_len);

	// Escribe la longitud en bits en big-endian
	uint8_t len_bytes[8];
	for (int i = 0; i < 8; ++i) {
		len_bytes[7 - i] = (uint8_t)(total_bits >> (i * 8));
	}
	sha_256_write(sha_256, len_bytes, 8);

	// Copia el estado interno al buffer de salida en big-endian
	for (int i = 0; i < 8; ++i) {
		sha_256->hash[i * 4 + 0] = (uint8_t)(sha_256->h[i] >> 24);
		sha_256->hash[i * 4 + 1] = (uint8_t)(sha_256->h[i] >> 16);
		sha_256->hash[i * 4 + 2] = (uint8_t)(sha_256->h[i] >> 8);
		sha_256->hash[i * 4 + 3] = (uint8_t)(sha_256->h[i]);
	}
	return sha_256->hash;
}

void calc_sha_256(uint8_t hash[SIZE_OF_SHA_256_HASH], const void *input, size_t len)
{
	struct Sha_256 sha_256;
	sha_256_init(&sha_256, hash);
	sha_256_write(&sha_256, input, len);
	(void)sha_256_close(&sha_256);
}