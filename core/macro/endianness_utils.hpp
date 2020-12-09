/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ENDIANNESS_UTILS_HPP
#define KAGOME_ENDIANNESS_UTILS_HPP

#ifdef _MSC_VER
#define LE_BE_SWAP32 _byteswap_ulong
#define LE_BE_SWAP64 _byteswap_uint64
#else  //_MSC_VER
#define LE_BE_SWAP32 __builtin_bswap32
#define LE_BE_SWAP64 __builtin_bswap64
#endif  //_MSC_VER

#endif  // KAGOME_ENDIANNESS_UTILS_HPP
