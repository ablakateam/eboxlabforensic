#ifndef CRYPTO_H
#define CRYPTO_H

#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdio>

namespace crypto {

// ══════════════════════════════════════════════════════════════════════════════
// SHA-256 (FIPS 180-4)
// ══════════════════════════════════════════════════════════════════════════════

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

struct SHA256_CTX {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
};

static inline uint32_t sha256_rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
static inline uint32_t sha256_ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static inline uint32_t sha256_maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline uint32_t sha256_sig0(uint32_t x) { return sha256_rotr(x,2) ^ sha256_rotr(x,13) ^ sha256_rotr(x,22); }
static inline uint32_t sha256_sig1(uint32_t x) { return sha256_rotr(x,6) ^ sha256_rotr(x,11) ^ sha256_rotr(x,25); }
static inline uint32_t sha256_ep0(uint32_t x) { return sha256_rotr(x,7) ^ sha256_rotr(x,18) ^ (x >> 3); }
static inline uint32_t sha256_ep1(uint32_t x) { return sha256_rotr(x,17) ^ sha256_rotr(x,19) ^ (x >> 10); }

static void sha256_transform(SHA256_CTX* ctx, const uint8_t data[]) {
    uint32_t w[64], a, b, c, d, e, f, g, h, t1, t2;
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)data[i*4] << 24) | ((uint32_t)data[i*4+1] << 16) |
               ((uint32_t)data[i*4+2] << 8) | (uint32_t)data[i*4+3];
    for (int i = 16; i < 64; i++)
        w[i] = sha256_ep1(w[i-2]) + w[i-7] + sha256_ep0(w[i-15]) + w[i-16];
    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
    for (int i = 0; i < 64; i++) {
        t1 = h + sha256_sig1(e) + sha256_ch(e,f,g) + sha256_k[i] + w[i];
        t2 = sha256_sig0(a) + sha256_maj(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static void sha256_init(SHA256_CTX* ctx) {
    ctx->datalen = 0; ctx->bitlen = 0;
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85;
    ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c;
    ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
}

static void sha256_update(SHA256_CTX* ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX* ctx, uint8_t hash[32]) {
    uint32_t i = ctx->datalen;
    ctx->data[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->data[i++] = 0;
        sha256_transform(ctx, ctx->data);
        i = 0;
    }
    while (i < 56) ctx->data[i++] = 0;
    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);
    for (int j = 0; j < 8; j++) {
        hash[j*4]   = (ctx->state[j] >> 24) & 0xff;
        hash[j*4+1] = (ctx->state[j] >> 16) & 0xff;
        hash[j*4+2] = (ctx->state[j] >> 8) & 0xff;
        hash[j*4+3] = ctx->state[j] & 0xff;
    }
}

[[maybe_unused]]
static std::string sha256_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "(error: cannot open file)";
    SHA256_CTX ctx;
    sha256_init(&ctx);
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        sha256_update(&ctx, (const uint8_t*)buf, (size_t)f.gcount());
        if (f.eof()) break;
    }
    uint8_t hash[32];
    sha256_final(&ctx, hash);
    char hex[65];
    for (int i = 0; i < 32; i++) std::sprintf(hex + i*2, "%02x", hash[i]);
    hex[64] = 0;
    return hex;
}

// ══════════════════════════════════════════════════════════════════════════════
// MD5 (RFC 1321)
// ══════════════════════════════════════════════════════════════════════════════

static const uint32_t md5_s[64] = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

static const uint32_t md5_k[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};

struct MD5_CTX {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[4];
};

static inline uint32_t md5_rotl(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

static void md5_transform(MD5_CTX* ctx, const uint8_t block[]) {
    uint32_t m[16];
    for (int i = 0; i < 16; i++)
        m[i] = ((uint32_t)block[i*4]) | ((uint32_t)block[i*4+1] << 8) |
               ((uint32_t)block[i*4+2] << 16) | ((uint32_t)block[i*4+3] << 24);
    uint32_t a = ctx->state[0], b = ctx->state[1], c = ctx->state[2], d = ctx->state[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f, g;
        if (i < 16)      { f = (b & c) | (~b & d);       g = (uint32_t)i; }
        else if (i < 32) { f = (d & b) | (~d & c);       g = (5*i + 1) % 16; }
        else if (i < 48) { f = b ^ c ^ d;                g = (3*i + 5) % 16; }
        else              { f = c ^ (b | ~d);             g = (7*i) % 16; }
        f = f + a + md5_k[i] + m[g];
        a = d; d = c; c = b; b = b + md5_rotl(f, md5_s[i]);
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
}

static void md5_init(MD5_CTX* ctx) {
    ctx->datalen = 0; ctx->bitlen = 0;
    ctx->state[0] = 0x67452301; ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe; ctx->state[3] = 0x10325476;
}

static void md5_update(MD5_CTX* ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            md5_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void md5_final(MD5_CTX* ctx, uint8_t hash[16]) {
    uint32_t i = ctx->datalen;
    ctx->data[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->data[i++] = 0;
        md5_transform(ctx, ctx->data);
        i = 0;
    }
    while (i < 56) ctx->data[i++] = 0;
    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    // Little-endian length
    ctx->data[56] = (uint8_t)(ctx->bitlen);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[63] = (uint8_t)(ctx->bitlen >> 56);
    md5_transform(ctx, ctx->data);
    for (int j = 0; j < 4; j++) {
        hash[j*4]   = ctx->state[j] & 0xff;
        hash[j*4+1] = (ctx->state[j] >> 8) & 0xff;
        hash[j*4+2] = (ctx->state[j] >> 16) & 0xff;
        hash[j*4+3] = (ctx->state[j] >> 24) & 0xff;
    }
}

[[maybe_unused]]
static std::string md5_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "(error: cannot open file)";
    MD5_CTX ctx;
    md5_init(&ctx);
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        md5_update(&ctx, (const uint8_t*)buf, (size_t)f.gcount());
        if (f.eof()) break;
    }
    uint8_t hash[16];
    md5_final(&ctx, hash);
    char hex[33];
    for (int i = 0; i < 16; i++) std::sprintf(hex + i*2, "%02x", hash[i]);
    hex[32] = 0;
    return hex;
}

} // namespace crypto

#endif
