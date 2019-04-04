/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/utils/base58_codec.hpp"

#include <cstring>
#include <vector>
#include <cmath>

#include "spdlog/spdlog.h"

using kagome::common::Buffer;

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi, Base58Codec::DecodeError, e) {
  using libp2p::multi::Base58Codec;  // not necessary, just for convenience
  switch (e) {
    case Base58Codec::DecodeError::kInvalidBase58Digit:
      return "The digit is not used in base58 encoding";
    case Base58Codec::DecodeError::kInvalidHighBit:
      return "High-bit set on invalid digit";
    case Base58Codec::DecodeError::kOutputTooBig:
      return "Output number too big";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi {

  std::string Base58Codec::encode(std::string_view str) {
    std::vector<uint8_t> v(str.begin(),
                           str.end());  //< to convert from char* to uint8_t*
                                        //without reinterpret cast
    return Base58Codec::encode(v);
  }

  std::string Base58Codec::encode(gsl::span<const uint8_t> bytes) {
    size_t zcount = 0;
    while (zcount < bytes.size() && bytes[zcount] == 0) {
      ++zcount;
    }

    auto size = static_cast<size_t>((bytes.size() - zcount) * 138 / 100 + 1);
    std::vector<uint8_t> buf(size, 0);

    int carry = 0;
    ssize_t i, j, high;
    for (i = zcount, high = size - 1; i < bytes.size(); ++i, high = j) {
      for (carry = bytes[i], j = size - 1; (j > high) || carry != 0; --j) {
        carry += 256 * buf[j];
        buf[j] = static_cast<uint8_t>(carry % 58);
        carry /= 58;
      }
    }

    for (j = 0; j < size && buf[j] == 0; ++j) {
    };

    std::string res(zcount + size - j + 1, '1');
    for (i = zcount; j < size; ++i, ++j) {
      res[i] = b58digits_ordered[buf[j]];
    }
    res.resize(i);
    return res;
  }

  auto Base58Codec::decode(std::string_view base58string)
      -> outcome::result<kagome::common::Buffer> {
    size_t res_size = calculateDecodedSize(base58string);
    std::vector<unsigned char> bin(res_size, 0);
    std::vector<unsigned char> b58u(base58string.begin(), base58string.end());
    size_t outisz = (res_size + 3) / 4;
    std::vector<uint32_t> outi(outisz, 0);
    uint64_t t;
    uint32_t c;
    size_t i, j;
    uint8_t bytesleft = res_size % 4;
    uint32_t zeromask = bytesleft ? (0xffffffff << (bytesleft * 8)) : 0;
    unsigned zerocount = 0;

    // Leading zeros, just count
    for (i = 0; i < base58string.size() && !b58digits_map.at(b58u[i]); ++i) {
      ++zerocount;
    }

    for (; i < base58string.size(); ++i) {
      if (b58u[i] & 0x80 == 0) {
        // High-bit set on invalid digit
        return DecodeError::kInvalidHighBit;
      }
      if (b58digits_map.at(b58u[i]) == -1) {
        // Invalid base58 digit
        return DecodeError::kInvalidBase58Digit;
      }
      c = (unsigned)b58digits_map.at(b58u[i]);
      for (j = outisz; j--;) {
        t = ((uint64_t)outi[j]) * 58 + c;
        c = (t & 0x3f00000000) >> 32;
        outi[j] = t & 0xffffffff;
      }
      if (c) {
        // Output number too big (carry to the next int32)
        return DecodeError::kOutputTooBig;
      }
      if (outi[0] & zeromask) {
        // Output number too big (last int32 filled too far)
        return DecodeError::kOutputTooBig;
      }
    }

    auto it = bin.begin();
    j = 0;
    switch (bytesleft) {
      case 3:
        *(it++) = (outi[0] & 0xff0000) >> 16;
      case 2:
        *(it++) = (outi[0] & 0xff00) >> 8;
      case 1:
        *(it++) = (outi[0] & 0xff);
        ++j;
      default:
        break;
    }

    for (; j < outisz; ++j) {
      *(it++) = (outi[j] >> 0x18) & 0xff;
      *(it++) = (outi[j] >> 0x10) & 0xff;
      *(it++) = (outi[j] >> 8) & 0xff;
      *(it++) = (outi[j] >> 0) & 0xff;
    }

    // Count canonical base58 byte count
    size_t s = res_size;
    for (i = 0; i < s; ++i) {
      if (bin[i]) {
        break;
      }
      --res_size;
    }
    res_size += zerocount;
    bin.resize(res_size);
    return Buffer{bin};
  }

  size_t Base58Codec::calculateDecodedSize(std::string_view base58string) {
    size_t string_length = base58string.size();
    size_t radix = b58digits_ordered.size();
    double bits_per_digit = log2(radix);

    return (size_t)ceil(string_length * bits_per_digit / 8);
  }

}  // namespace libp2p::multi
