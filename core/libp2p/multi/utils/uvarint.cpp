/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/utils/uvarint.hpp"
#include "common/hexutil.hpp"

using kagome::common::hex_upper;

namespace libp2p::multi {

  UVarint::UVarint(uint64_t number) {
    bytes_.resize(8);
    size_t i = 0;
    size_t size = 0;
    for (; i < 8; i++) {
      bytes_[i] = (uint8_t)((number & 0xFFul) | 0x80ul);
      number >>= 7ul;
      if (!number) {
        bytes_[i] &= 0x7Ful;
        size = i + 1;
        break;
      }
    }
    bytes_.resize(size);
  }

  UVarint::UVarint(gsl::span<const uint8_t> varint_bytes)
      : bytes_(varint_bytes.begin(),
               varint_bytes.begin() + calculateSize(varint_bytes)) {}

  uint64_t UVarint::toUInt64() const {
    uint64_t res = 0;
    for (size_t i = 0; i < 8 && i < bytes_.size(); i++) {
      res |= ((bytes_[i] & 0x7ful) << (7 * i));
      if (!(bytes_[i] & 0x80ul))
        return res;
    }
    return -1;
  }

  gsl::span<const uint8_t> UVarint::toBytes() const {
    return gsl::span(bytes_.data(), bytes_.size());
  }

  std::string UVarint::toHex() const {
    return hex_upper(bytes_);
  }

  size_t UVarint::size() const {
    return bytes_.size();
  }

  UVarint &UVarint::operator=(uint64_t n) {
    *this = UVarint(n);
    return *this;
  }

  size_t UVarint::calculateSize(gsl::span<const uint8_t> varint_bytes) {
    size_t s = 0;

    while ((varint_bytes[s] & 0x80) != 0) {
      s++;
    }
    return s + 1;
  }

}  // namespace libp2p::multi
