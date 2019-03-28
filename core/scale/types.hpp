/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPES_HPP
#define KAGOME_SCALE_TYPES_HPP

#include <cstdint>
#include <vector>

#include <boost/logic/tribool.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "common/result.hpp"

namespace kagome::common {
  /**
   * @brief interface for Decoders
   */
  class Stream {
   public:
    virtual ~Stream() = default;

    /**
     * @brief hasMore Checks whether n more bytes are available
     * @param n Number of bytes to check
     * @return True if n more bytes are available and false otherwise
     */
    virtual bool hasMore(uint64_t n) const = 0;

    /**
     * @brief nextByte Takes current byte and moves pointer to the next one
     * @return Current byte
     */
    virtual std::optional<uint8_t> nextByte() = 0;
  };

  namespace scale {
    using ByteArray = std::vector<uint8_t>;
    using BigInteger = boost::multiprecision::cpp_int;

    using tribool = boost::logic::tribool;

    constexpr auto indeterminate = boost::logic::indeterminate;
    constexpr auto isIndeterminate = [](tribool value) {
      return boost::logic::indeterminate(value);
    };
  }  // namespace scale
}  // namespace kagome::common

#endif  // KAGOME_SCALE_TYPES_HPP
