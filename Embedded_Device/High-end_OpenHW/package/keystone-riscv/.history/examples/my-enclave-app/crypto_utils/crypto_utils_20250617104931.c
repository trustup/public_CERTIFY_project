#include "crypto_utils.h"
// #include <string.h>

// // Declaraciones de funciones estáticas
// static void md5_transform(uint32_t state[4], const uint8_t block[64]);
// static void encode(uint8_t *output, const uint32_t *input, size_t len);
// static void decode(uint32_t *output, const uint8_t *input, size_t len);

// // Constante de padding para MD5
// static const uint8_t PADDING[64] = {
//     0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
// };

// // MD5 Constants
// #define S11 7
// #define S12 12
// #define S13 17
// #define S14 22
// #define S21 5
// #define S22 9
// #define S23 14
// #define S24 20
// #define S31 4
// #define S32 11
// #define S33 16
// #define S34 23
// #define S41 6
// #define S42 10
// #define S43 15
// #define S44 21

// // F, G, H and I are basic MD5 functions
// #define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
// #define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
// #define H(x, y, z) ((x) ^ (y) ^ (z))
// #define I(x, y, z) ((y) ^ ((x) | (~z)))

// // ROTATE_LEFT rotates x left n bits
// #define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// // FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4
// #define FF(a, b, c, d, x, s, ac) { \
//     (a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac); \
//     (a) = ROTATE_LEFT ((a), (s)); \
//     (a) += (b); \
// }
// #define GG(a, b, c, d, x, s, ac) { \
//     (a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac); \
//     (a) = ROTATE_LEFT ((a), (s)); \
//     (a) += (b); \
// }
// #define HH(a, b, c, d, x, s, ac) { \
//     (a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac); \
//     (a) = ROTATE_LEFT ((a), (s)); \
//     (a) += (b); \
// }
// #define II(a, b, c, d, x, s, ac) { \
//     (a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac); \
//     (a) = ROTATE_LEFT ((a), (s)); \
//     (a) += (b); \
// }

// // MD5 initialization
// void md5_init(MD5_CTX *ctx) {
//     ctx->count[0] = ctx->count[1] = 0;
//     ctx->state[0] = 0x67452301;
//     ctx->state[1] = 0xefcdab89;
//     ctx->state[2] = 0x98badcfe;
//     ctx->state[3] = 0x10325476;
// }

// // MD5 block update operation
// void md5_update(MD5_CTX *ctx, const uint8_t *data, size_t len) {
//     size_t i, index, partLen;

//     index = (size_t)((ctx->count[0] >> 3) & 0x3F);

//     if ((ctx->count[0] += (len << 3)) < (len << 3))
//         ctx->count[1]++;
//     ctx->count[1] += (len >> 29);

//     partLen = 64 - index;

//     if (len >= partLen) {
//         memcpy(&ctx->buffer[index], data, partLen);
//         md5_transform(ctx->state, ctx->buffer);

//         for (i = partLen; i + 63 < len; i += 64)
//             md5_transform(ctx->state, &data[i]);

//         index = 0;
//     } else
//         i = 0;

//     memcpy(&ctx->buffer[index], &data[i], len - i);
// }

// // MD5 finalization
// void md5_final(uint8_t digest[MD5_DIGEST_SIZE], MD5_CTX *ctx) {
//     uint8_t bits[8];
//     size_t oldState[4];
//     size_t oldCount[2];
//     size_t index, padLen;

//     memcpy(oldState, ctx->state, 16);
//     memcpy(oldCount, ctx->count, 8);

//     encode(bits, ctx->count, 8);

//     index = (size_t)((ctx->count[0] >> 3) & 0x3f);
//     padLen = (index < 56) ? (56 - index) : (120 - index);
//     md5_update(ctx, PADDING, padLen);

//     md5_update(ctx, bits, 8);

//     encode(digest, ctx->state, 16);

//     memcpy(ctx->state, oldState, 16);
//     memcpy(ctx->count, oldCount, 8);
// }

// // MD5 basic transformation
// static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
//     uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

//     decode(x, block, 64);

//     // Round 1
//     FF(a, b, c, d, x[0], S11, 0xd76aa478);
//     FF(d, a, b, c, x[1], S12, 0xe8c7b756);
//     FF(c, d, a, b, x[2], S13, 0x242070db);
//     FF(b, c, d, a, x[3], S14, 0xc1bdceee);
//     FF(a, b, c, d, x[4], S11, 0xf57c0faf);
//     FF(d, a, b, c, x[5], S12, 0x4787c62a);
//     FF(c, d, a, b, x[6], S13, 0xa8304613);
//     FF(b, c, d, a, x[7], S14, 0xfd469501);
//     FF(a, b, c, d, x[8], S11, 0x698098d8);
//     FF(d, a, b, c, x[9], S12, 0x8b44f7af);
//     FF(c, d, a, b, x[10], S13, 0xffff5bb1);
//     FF(b, c, d, a, x[11], S14, 0x895cd7be);
//     FF(a, b, c, d, x[12], S11, 0x6b901122);
//     FF(d, a, b, c, x[13], S12, 0xfd987193);
//     FF(c, d, a, b, x[14], S13, 0xa679438e);
//     FF(b, c, d, a, x[15], S14, 0x49b40821);

//     // Round 2
//     GG(a, b, c, d, x[1], S21, 0xf61e2562);
//     GG(d, a, b, c, x[6], S22, 0xc040b340);
//     GG(c, d, a, b, x[11], S23, 0x265e5a51);
//     GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
//     GG(a, b, c, d, x[5], S21, 0xd62f105d);
//     GG(d, a, b, c, x[10], S22, 0x2441453);
//     GG(c, d, a, b, x[15], S23, 0xd8a1e681);
//     GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
//     GG(a, b, c, d, x[9], S21, 0x21e1cde6);
//     GG(d, a, b, c, x[14], S22, 0xc33707d6);
//     GG(c, d, a, b, x[3], S23, 0xf4d50d87);
//     GG(b, c, d, a, x[8], S24, 0x455a14ed);
//     GG(a, b, c, d, x[13], S21, 0xa9e3e905);
//     GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
//     GG(c, d, a, b, x[7], S23, 0x676f02d9);
//     GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

//     // Round 3
//     HH(a, b, c, d, x[5], S31, 0xfffa3942);
//     HH(d, a, b, c, x[8], S32, 0x8771f681);
//     HH(c, d, a, b, x[11], S33, 0x6d9d6122);
//     HH(b, c, d, a, x[14], S34, 0xfde5380c);
//     HH(a, b, c, d, x[1], S31, 0xa4beea44);
//     HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
//     HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
//     HH(b, c, d, a, x[10], S34, 0xbebfbc70);
//     HH(a, b, c, d, x[13], S31, 0x289b7ec6);
//     HH(d, a, b, c, x[0], S32, 0xeaa127fa);
//     HH(c, d, a, b, x[3], S33, 0xd4ef3085);
//     HH(b, c, d, a, x[6], S34, 0x4881d05);
//     HH(a, b, c, d, x[9], S31, 0xd9d4d039);
//     HH(d, a, b, c, x[12], S32, 0xe6db99e5);
//     HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
//     HH(b, c, d, a, x[2], S34, 0xc4ac5665);

//     // Round 4
//     II(a, b, c, d, x[0], S41, 0xf4292244);
//     II(d, a, b, c, x[7], S42, 0x432aff97);
//     II(c, d, a, b, x[14], S43, 0xab9423a7);
//     II(b, c, d, a, x[5], S44, 0xfc93a039);
//     II(a, b, c, d, x[12], S41, 0x655b59c3);
//     II(d, a, b, c, x[3], S42, 0x8f0ccc92);
//     II(c, d, a, b, x[10], S43, 0xffeff47d);
//     II(b, c, d, a, x[1], S44, 0x85845dd1);
//     II(a, b, c, d, x[8], S41, 0x6fa87e4f);
//     II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
//     II(c, d, a, b, x[6], S43, 0xa3014314);
//     II(b, c, d, a, x[13], S44, 0x4e0811a1);
//     II(a, b, c, d, x[4], S41, 0xf7537e82);
//     II(d, a, b, c, x[11], S42, 0xbd3af235);
//     II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
//     II(b, c, d, a, x[9], S44, 0xeb86d391);

//     state[0] += a;
//     state[1] += b;
//     state[2] += c;
//     state[3] += d;
// }

// // HMAC-MD5 implementation
// void hmac_md5_init(MD5_CTX *ctx, const uint8_t *key, size_t key_len) {
//     uint8_t k_ipad[MD5_BLOCK_SIZE];
//     uint8_t k_opad[MD5_BLOCK_SIZE];
//     uint8_t tk[MD5_DIGEST_SIZE];
//     int i;

//     if (key_len > MD5_BLOCK_SIZE) {
//         MD5_CTX tctx;
//         md5_init(&tctx);
//         md5_update(&tctx, key, key_len);
//         md5_final(tk, &tctx);
//         key = tk;
//         key_len = MD5_DIGEST_SIZE;
//     }

//     memset(k_ipad, 0, sizeof k_ipad);
//     memset(k_opad, 0, sizeof k_opad);
//     memcpy(k_ipad, key, key_len);
//     memcpy(k_opad, key, key_len);

//     for (i = 0; i < MD5_BLOCK_SIZE; i++) {
//         k_ipad[i] ^= 0x36;
//         k_opad[i] ^= 0x5c;
//     }

//     md5_init(ctx);
//     md5_update(ctx, k_ipad, MD5_BLOCK_SIZE);
// }

// void hmac_md5_update(MD5_CTX *ctx, const uint8_t *data, size_t len) {
//     md5_update(ctx, data, len);
// }

// void hmac_md5_final(uint8_t digest[MD5_DIGEST_SIZE], MD5_CTX *ctx, const uint8_t *key, size_t key_len) {
//     uint8_t tk[MD5_DIGEST_SIZE];
//     uint8_t k_opad[MD5_BLOCK_SIZE];
//     MD5_CTX tctx;
//     int i;

//     if (key_len > MD5_BLOCK_SIZE) {
//         md5_init(&tctx);
//         md5_update(&tctx, key, key_len);
//         md5_final(tk, &tctx);
//         key = tk;
//         key_len = MD5_DIGEST_SIZE;
//     }

//     memset(k_opad, 0, sizeof k_opad);
//     memcpy(k_opad, key, key_len);

//     for (i = 0; i < MD5_BLOCK_SIZE; i++)
//         k_opad[i] ^= 0x5c;

//     md5_final(digest, ctx);

//     md5_init(&tctx);
//     md5_update(&tctx, k_opad, MD5_BLOCK_SIZE);
//     md5_update(&tctx, digest, MD5_DIGEST_SIZE);
//     md5_final(digest, &tctx);
// }

// void hmac_md5(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t digest[MD5_DIGEST_SIZE]) {
//     MD5_CTX ctx;
//     hmac_md5_init(&ctx, key, key_len);
//     hmac_md5_update(&ctx, data, data_len);
//     hmac_md5_final(digest, &ctx, key, key_len);
// }

// // Helper functions for MD5
// static void encode(uint8_t *output, const uint32_t *input, size_t len) {
//     size_t i, j;
//     for (i = 0, j = 0; j < len; i++, j += 4) {
//         output[j] = (uint8_t)(input[i] & 0xff);
//         output[j+1] = (uint8_t)((input[i] >> 8) & 0xff);
//         output[j+2] = (uint8_t)((input[i] >> 16) & 0xff);
//         output[j+3] = (uint8_t)((input[i] >> 24) & 0xff);
//     }
// }

// static void decode(uint32_t *output, const uint8_t *input, size_t len) {
//     size_t i, j;
//     for (i = 0, j = 0; j < len; i++, j += 4)
//         output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j+1]) << 8) |
//                    (((uint32_t)input[j+2]) << 16) | (((uint32_t)input[j+3]) << 24);
// }

// // CMAC-AES Constants
// #define AES_BLOCK_SIZE 16
// #define RB 0x87

// // CMAC-AES implementation
// static void xor_128(const uint8_t *a, const uint8_t *b, uint8_t *out) {
//     for (int i = 0; i < AES_BLOCK_SIZE; i++) {
//         out[i] = a[i] ^ b[i];
//     }
// }

// static void leftshift_onebit(const uint8_t *input, uint8_t *output) {
//     int i;
//     uint8_t overflow = 0;

//     for (i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
//         output[i] = input[i] << 1;
//         output[i] |= overflow;
//         overflow = (input[i] & 0x80) ? 1 : 0;
//     }
// }

// static void generate_subkey(struct AES_ctx *ctx, uint8_t *k1, uint8_t *k2) {
//     uint8_t l[AES_BLOCK_SIZE];
//     uint8_t z[AES_BLOCK_SIZE] = {0};
    
//     // Generar L = E(K, 0)
//     AES_ECB_encrypt(ctx, z);
//     memcpy(l, z, AES_BLOCK_SIZE);
    
//     // Generar K1
//     if ((l[0] & 0x80) == 0) {
//         leftshift_onebit(k1, l);
//     } else {
//         leftshift_onebit(k1, l);
//         k1[15] ^= RB;
//     }
    
//     // Generar K2
//     if ((k1[0] & 0x80) == 0) {
//         leftshift_onebit(k2, k1);
//     } else {
//         leftshift_onebit(k2, k1);
//         k2[15] ^= RB;
//     }
// }

// void cmac_aes_init(struct AES_ctx *ctx, const uint8_t *key, size_t key_len) {
//     AES_init_ctx(ctx, key);
// }

// void cmac_aes_update(struct AES_ctx *ctx, const uint8_t *data, size_t len, uint8_t *mac) {
//     size_t i;
//     uint8_t x[AES_BLOCK_SIZE] = {0};
//     uint8_t k1[AES_BLOCK_SIZE], k2[AES_BLOCK_SIZE];
    
//     generate_subkey(ctx, k1, k2);
    
//     // Procesar bloques completos
//     for (i = 0; i < len - AES_BLOCK_SIZE; i += AES_BLOCK_SIZE) {
//         xor_128(x, data + i);
//         AES_ECB_encrypt(ctx, x);
//     }
    
//     // Procesar último bloque
//     if (len % AES_BLOCK_SIZE == 0) {
//         xor_128(x, data + i);
//         xor_128(x, k1);
//     } else {
//         // Padding del último bloque
//         uint8_t padded[AES_BLOCK_SIZE] = {0};
//         memcpy(padded, data + i, len - i);
//         padded[len - i] = 0x80;
//         xor_128(x, padded);
//         xor_128(x, k2);
//     }
//     AES_ECB_encrypt(ctx, x);
//     memcpy(mac, x, AES_BLOCK_SIZE);
// }

// void cmac_aes_final(struct AES_ctx *ctx, uint8_t *mac) {
//     // No additional operations needed for finalization in CMAC
//     // The MAC is already in the mac buffer from the last update
// }

// void cmac_aes(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t mac[16]) {
//     struct AES_ctx ctx;
//     cmac_aes_init(&ctx, key, key_len);
//     cmac_aes_update(&ctx, data, data_len, mac);
//     cmac_aes_final(&ctx, mac);
// } 