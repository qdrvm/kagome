/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sstream>

#include "libp2p/multi/utils/uvarint.hpp"
#include "common/hexutil.hpp"
extern "C" {
#include "libp2p/multi/c-utils/varint.h"
}

using kagome::common::hex_upper;

namespace libp2p::multi {

  UVarint::UVarint(uint64_t number):
    bytes_ {8, 0} {
    auto size = uvarint_encode64(number, bytes_.data(), 8);
    bytes_.resize(size);
  }

  UVarint::UVarint(gsl::span<const uint8_t> varint_bytes)
      : bytes_(varint_bytes.begin(),
               varint_bytes.begin() + calculateSize(varint_bytes)) {}

  uint64_t UVarint::toUInt64() const {
    uint64_t i;
    uvarint_decode64(bytes_.data(), bytes_.size(), &i);
    return i;
  }

  gsl::span<const uint8_t> UVarint::toBytes() const {
    return gsl::span(bytes_.data(), bytes_.size());
  }

  size_t UVarint::size() const {
    return bytes_.size();
  }

  UVarint &UVarint::operator=(uint64_t n) {
    bytes_.resize(8);
    auto size = uvarint_encode64(n, bytes_.data(), 8);
    bytes_.resize(size);
    return *this;
  }

  size_t UVarint::calculateSize(gsl::span<const uint8_t> varint_bytes) {
    size_t s = 0;

    while ((varint_bytes[s] & 0x80) != 0) {
      s++;
    }
    return s + 1;
  }

  std::string UVarint::toHex() const {
    return hex_upper(toBytes().data(), static_cast<size_t>(toBytes().size()));
  }

}  // namespace libp2p::multi
