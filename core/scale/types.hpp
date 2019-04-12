/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPES_HPP
#define KAGOME_SCALE_TYPES_HPP

#include <vector>

#include <boost/logic/tribool.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <outcome/outcome.hpp>

namespace kagome::scale {
  using ByteArray = std::vector<uint8_t>;
  using BigInteger = boost::multiprecision::cpp_int;

  using tribool = boost::logic::tribool;

  constexpr auto indeterminate = boost::logic::indeterminate;
  constexpr auto isIndeterminate(tribool value) {
    return boost::logic::indeterminate(value);
  }
}  // namespace kagome::scale

#endif  // KAGOME_SCALE_TYPES_HPP
