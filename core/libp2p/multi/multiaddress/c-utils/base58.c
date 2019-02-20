/*
 * Copyright 2012-2014 Luke Dashjr
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the standard MIT license.  See COPYING for more details.
 */

#include <string.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>

static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const int8_t b58digits_map[] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};

/**
 * convert a base58 encoded string into a binary array
 * @param b58 the base58 encoded string
 * @param base58_size the size of the encoded string
 * @param bin the results buffer
 * @param binszp the size of the results buffer
 * @returns true(1) on success
 */
int multiaddr_encoding_base58_decode(const char* b58, size_t base58_size, unsigned char** bin, size_t* binszp)
{
	size_t binsz = *binszp;
	const unsigned char* b58u = (const void*)b58;
	unsigned char* binu = *bin;
	size_t outisz = (binsz + 3) / 4;
	uint32_t outi[outisz];
	uint64_t t;
	uint32_t c;
	size_t i, j;
	uint8_t bytesleft = binsz % 4;
	uint32_t zeromask = bytesleft ? (0xffffffff << (bytesleft * 8)) : 0;
	unsigned zerocount = 0;
	size_t b58sz;
	
	b58sz = strlen(b58);
	
	memset(outi, 0, outisz * sizeof(*outi));
	
	// Leading zeros, just count
	for (i = 0; i < b58sz && !b58digits_map[b58u[i]]; ++i) {
		++zerocount;
	}
	
	for (; i < b58sz; ++i) {
		if (b58u[i] & 0x80) {
			// High-bit set on invalid digit
			return 0;
		}
		if (b58digits_map[b58u[i]] == -1) {
			// Invalid base58 digit
			return 0;
		}
		c = (unsigned)b58digits_map[b58u[i]];
		for (j = outisz; j--;) {
			t = ((uint64_t)outi[j]) * 58 + c;
			c = (t & 0x3f00000000) >> 32;
			outi[j] = t & 0xffffffff;
		}
		if (c) {
			// Output number too big (carry to the next int32)
			memset(outi, 0, outisz * sizeof(*outi));
			return 0;
		}
		if (outi[0] & zeromask) {
			// Output number too big (last int32 filled too far)
			memset(outi, 0, outisz * sizeof(*outi));
			return 0;
		}
	}
	
	j = 0;
	switch (bytesleft) {
		case 3:
			*(binu++) = (outi[0] & 0xff0000) >> 16;
		case 2:
			*(binu++) = (outi[0] & 0xff00) >> 8;
		case 1:
			*(binu++) = (outi[0] & 0xff);
			++j;
		default:
			break;
	}
	
	for (; j < outisz; ++j) {
		*(binu++) = (outi[j] >> 0x18) & 0xff;
		*(binu++) = (outi[j] >> 0x10) & 0xff;
		*(binu++) = (outi[j] >> 8) & 0xff;
		*(binu++) = (outi[j] >> 0) & 0xff;
	}
	
	// Count canonical base58 byte count
	binu = *bin;
	for (i = 0; i < binsz; ++i) {
		if (binu[i]) {
			break;
		}
		--*binszp;
	}
	*binszp += zerocount;
	
	memset(outi, 0, outisz * sizeof(*outi));
	return 1;
}

/**
 * encode an array of bytes into a base58 string
 * @param binary_data the data to be encoded
 * @param binary_data_size the size of the data to be encoded
 * @param base58 the results buffer
 * @param base58_size the size of the results buffer
 * @returns true(1) on success
 */
int multiaddr_encoding_base58_encode(const unsigned char* data, size_t binsz, unsigned char** b58, size_t* b58sz)
{
	const uint8_t* bin = data;
	int carry;
	ssize_t i, j, high, zcount = 0;
	size_t size;
	
	while (zcount < (ssize_t)binsz && !bin[zcount]) {
		++zcount;
	}
	
	size = (binsz - zcount) * 138 / 100 + 1;
	uint8_t buf[size];
	memset(buf, 0, size);
	
	for (i = zcount, high = size - 1; i < (ssize_t)binsz; ++i, high = j) {
		for (carry = bin[i], j = size - 1; (j > high) || carry; --j) {
			carry += 256 * buf[j];
			buf[j] = carry % 58;
			carry /= 58;
		}
	}
	
	for (j = 0; j < (ssize_t)size && !buf[j]; ++j)
		;
	
	if (*b58sz <= zcount + size - j) {
		*b58sz = zcount + size - j + 1;
		memset(buf, 0, size);
		return 0;
	}
	
	if (zcount) {
		memset(b58, '1', zcount);
	}
	for (i = zcount; j < (ssize_t)size; ++i, ++j) {
		(*b58)[i] = b58digits_ordered[buf[j]];
	}
	(*b58)[i] = '\0';
	*b58sz = i + 1;
	
	memset(buf, 0, size);
	return 1;
}

/***
 * calculate the size of the binary results based on an incoming base58 string
 * @param base58_string the string
 * @returns the size in bytes had the string been decoded
 */
size_t multiaddr_encoding_base58_decode_size(const unsigned char* base58_string) {
	size_t string_length = strlen((char*)base58_string);
	size_t decoded_length = 0;
	size_t radix = strlen(b58digits_ordered);
	double bits_per_digit = log2(radix);
	
	decoded_length = floor(string_length * bits_per_digit / 8);
	return decoded_length;
}

/**
 * calculate the max length in bytes of an encoding of n source bits
 * @param base58_string the string
 * @returns the maximum size in bytes had the string been decoded
 */
size_t multiaddr_encoding_base58_decode_max_size(const unsigned char* base58_string) {
	size_t string_length = strlen((char*)base58_string);
	size_t decoded_length = 0;
	size_t radix = strlen(b58digits_ordered);
	double bits_per_digit = log2(radix);
	
	decoded_length = ceil(string_length * bits_per_digit / 8);
	return decoded_length;
}
