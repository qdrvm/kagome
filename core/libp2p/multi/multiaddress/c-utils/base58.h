#ifndef base58_h
#define base58_h
#include "libp2p/multi/multiaddress/c-utils/varint.h"
/**
 * convert a base58 encoded string into a binary array
 * @param base58 the base58 encoded string
 * @param base58_size the size of the encoded string
 * @param binary_data the results buffer
 * @param binary_data_size the size of the results buffer
 * @returns true(1) on success
 */
int multiaddr_encoding_base58_decode(const char *base58,
                                     size_t base58_size,
                                     unsigned char **binary_data,
                                     size_t *binary_data_size);

/**
 * encode an array of bytes into a base58 string
 * @param binary_data the data to be encoded
 * @param binary_data_size the size of the data to be encoded
 * @param base58 the results buffer
 * @param base58_size the size of the results buffer
 * @returns true(1) on success
 */
int multiaddr_encoding_base58_encode(const unsigned char *binary_data,
                                     size_t binary_data_size,
                                     unsigned char **base58,
                                     size_t *base58_size);

/***
 * calculate the size of the binary results based on an incoming base58 string
 * with no initial padding
 * @param base58_string the string
 * @returns the size in bytes had the string been decoded
 */
size_t multiaddr_encoding_base58_decode_size(
    const unsigned char *base58_string);

/**
 * calculate the max length in bytes of an encoding of n source bits
 * @param base58_string the string
 * @returns the maximum size in bytes had the string been decoded
 */
size_t multiaddr_encoding_base58_decode_max_size(
    const unsigned char *base58_string);

#endif /* base58_h */
