#ifndef POLKADOT_BLAKE2S_H
#define POLKADOT_BLAKE2S_H

// blake2s.h
// BLAKE2s Hashing Context and API Prototypes

#ifndef BLAKE2S_H
#define BLAKE2S_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// state context
typedef struct {
  uint8_t b[64];                      // input buffer
  uint32_t h[8];                      // chained state
  uint32_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
} blake2s_ctx;

// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).
int blake2s_init(blake2s_ctx *ctx, size_t outlen,
                 const void *key, size_t keylen);    // secret key

int blake2s_256_init(blake2s_ctx *ctx);

// Add "inlen" bytes from "in" into the hash.
void blake2s_update(blake2s_ctx *ctx,   // context
                    const void *in, size_t inlen);      // data to be hashed

// Generate the message digest (size given in init).
//      Result placed in "out".
void blake2s_final(blake2s_ctx *ctx, void *out);

// All-in-one convenience function.
int blake2s(void *out, size_t outlen,   // return buffer for digest
            const void *key, size_t keylen,     // optional secret key
            const void *in, size_t inlen);      // data to be hashed

// all-n-one convenience function, no key, 256 bit
int blake2s_256(void *out, const void *in, size_t inlen);

#if defined(__cplusplus)
}
#endif

#endif

#endif //POLKADOT_BLAKE2S_H
