/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CORE_KECCAK_H
#define CORE_KECCAK_H

#if defined(__cplusplus)
extern "C" {
#endif

/* 'Words' here refers to uint64_t */
#define SHA3_KECCAK_SPONGE_WORDS \
  (((1600) / 8 /*bits to byte*/) / sizeof(uint64_t))
typedef struct sha3_context_ {
  uint64_t saved; /* the portion of the input message that we
                   * didn't consume yet */
  union {         /* Keccak's state */
    uint64_t s[SHA3_KECCAK_SPONGE_WORDS];
    uint8_t sb[SHA3_KECCAK_SPONGE_WORDS * 8];
  };
  unsigned byteIndex;     /* 0..7--the next byte after the set one
                           * (starts from 0; 0--none are buffered) */
  unsigned wordIndex;     /* 0..24--the next word to integrate input
                           * (starts from 0) */
  unsigned capacityWords; /* the double size of the hash output in
                           * words (e.g. 16 for Keccak 512) */
} sha3_context;

enum SHA3_FLAGS { SHA3_FLAGS_NONE = 0, SHA3_FLAGS_KECCAK = 1 };

enum SHA3_RETURN { SHA3_RETURN_OK = 0, SHA3_RETURN_BAD_PARAMS = 1 };
typedef enum SHA3_RETURN sha3_return_t;

/* For Init or Reset call these: */
sha3_return_t sha3_Init(void *priv, unsigned bitSize);

void sha3_Init256(void *priv);
void sha3_Init384(void *priv);
void sha3_Init512(void *priv);

enum SHA3_FLAGS sha3_SetFlags(void *priv, enum SHA3_FLAGS);

void sha3_Update(void *priv, void const *bufIn, size_t len);

void const *sha3_Finalize(void *priv);

/* Single-call hashing */
sha3_return_t sha3_HashBuffer(
    unsigned bitSize,      /* 256, 384, 512 */
    enum SHA3_FLAGS flags, /* SHA3_FLAGS_NONE or SHA3_FLAGS_KECCAK */
    const void *in,
    unsigned inBytes,
    void *out,
    unsigned outBytes); /* up to bitSize/8; truncation OK */

#if defined(__cplusplus)
}
#endif

#endif
