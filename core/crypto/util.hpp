/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_UTIL_HPP
#define KAGOME_CORE_CRYPTO_UTIL_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>

namespace kagome::crypto::util {

  std::array<uint8_t, 32> uint256_t_to_bytes(
      const boost::multiprecision::uint256_t &i);

  boost::multiprecision::uint256_t bytes_to_uint256_t(
      gsl::span<uint8_t, 32> bytes);

}  // namespace kagome::crypto::util

#endif  // KAGOME_CORE_CRYPTO_UTIL_HPP
