/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_HPP
#define KAGOME_SCALE_HPP

/**
 * Scale namespace contains functions and classes
 * for encoding and decoding values in and from SCALE encoding.
 * There is no way to specify what exactly data is encoded,
 * and specification has no mentions concerning this fact.
 * However the SUBSTRATE documentation says directly, that
 * SCALE encoding is not self-describing and that encoding context
 * has to know what data it needs to decode.
 * In this concern, signed and unsigned integers encoded the same way.
 * When decoding you have to reinterpret integer value as signed
 * if you know it is signed.
 *
 * This library provides encoding and decoding following types:
 * ** Fixed width signed and unsigned 1,2,4 byte integers
 *    encoded little-endian
 *
 * ** Unsigned big integers in range 0..2^536-1
 *    use compact encoding, number of encoded bytes varies from 1 to 67
 *    depending on value
 *
 * ** Boolean values: false -> 0x00,
 *                    true  -> 0x01
 *
 * ** Tribool values: false is 0x00, true is 0x01, indeterminate is 0x02
 *
 * ** Collections of same type values
 *    Just like arrays they start with compact-encoded integer
 *    representing number of items in collection.
 *    After that collection of encoded items one by one,
 *    encoding depends on item type
 *
 * ** Optional values are encoded as follows:
 *    first byte is 0 when no value is provided, 1 - otherwise
 *    following bytes represent encoded values
 *
 *    Optional bool is a special case,
 *    they need only one byte: 0 - No value, 1 - false, 2 - true
 *
 * ** Tuples and structures (NOT IMPLEMENTED YET)
 *    Are simply concatenation of ordered encoded values
 *
 * ** Variant values (NOT IMPLEMENTED YET)
 *    Variants are encoded as follows:
 *    first byte represents index of type in varian
 *    following bytes are encoded value itself.
 *
 * The library consists of following header each representing own part
 * of functionality, and they can be used independently:

 * Fixed width integers operations
 * common/scale/fixedwidth.hpp
 *
 * Base compact operations:
 * common/scale/compact.hpp
 *
 * Boolean values operations:
 * common/scale/boolean.hpp
 *
 * Collection
 * common/scale/collection.hpp
 *
 * Optionals values operations:
 * common/scale/optional.hpp
 *
 * Tuples are simply concatenation of values
 * NOT IMPLEMENTED YET
 *
 * Structures are user-defined type and need
 * to implement specialized TypeDecoder<>
 * Examples NOT IMPLEMENTED YET
 *
 */

#include "scale/boolean.hpp"
#include "scale/collection.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/optional.hpp"

#endif  // KAGOME_SCALE_HPP
