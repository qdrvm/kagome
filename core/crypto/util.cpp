/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/util.hpp"

namespace kagome::crypto::util {

  std::array<uint8_t, 32> uint256_t_to_bytes(
      const boost::multiprecision::uint256_t &i) {
    auto count = i.backend().size();
    auto tsize = sizeof(boost::multiprecision::limb_type);
    auto copy_count = count * tsize;

    std::array<uint8_t, 32> output{};
    memcpy(output.data(), i.backend().limbs(), copy_count);
    return output;
  }

  boost::multiprecision::uint256_t bytes_to_uint256_t(
      gsl::span<uint8_t, 32> bytes) {
    boost::multiprecision::uint256_t i;
    auto size = bytes.size();
    i.backend().resize(size, size);
    memcpy(i.backend().limbs(), bytes.data(), bytes.size());
    i.backend().normalize();
    return i;
  }

}  // namespace kagome::crypto::util
