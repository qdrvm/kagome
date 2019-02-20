#ifndef VARINT
#define VARINT
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint8_t, uint64_t */


#define DEFN_ENCODER(SIZE) \
  size_t \
  uvarint_encode##SIZE (uint##SIZE##_t val, uint8_t buf[], size_t bufsize) \
  { \
    size_t i = 0; \
    for (; i < (SIZE/8) && i < bufsize; i++) { \
      buf[i] = (uint8_t) ((val & 0xFF) | 0x80); \
      val >>= 7; \
      if (!val) \
        return i + 1; \
    } \
    return -1; \
  }


#define DEFN_DECODER(SIZE) \
  size_t \
  uvarint_decode##SIZE (uint8_t buf[], size_t bufsize, uint##SIZE##_t *val) \
  { \
    *val = 0; \
    size_t i = 0; \
    for (; i < (SIZE/8) && i < bufsize; i++) { \
      *val |= ((buf[i] & 0x7f) << (7 * i)); \
      if (!(buf[i] & 0x80)) \
        return i + 1; \
    } \
    return -1; \
  }


#define DECL_ENCODER(SIZE) \
  size_t \
  uvarint_encode##SIZE (uint##SIZE##_t val, uint8_t buf[], size_t bufsize);

#define DECL_DECODER(SIZE) \
  size_t \
  uvarint_decode##SIZE (uint8_t buf[], size_t bufsize, uint##SIZE##_t *val);


DECL_ENCODER(32)
DECL_DECODER(32)

DECL_ENCODER(64)
DECL_DECODER(64)
#endif