/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_TIME_HPP
#define KAGOME_CORE_COMMON_TIME_HPP

// copied from
// https://github.com/hyperledger/iroha/blob/1.0.1/libs/datetime/time.hpp

#include <chrono>

namespace kagome {

  // timestamps
  using ts64_t = uint64_t;
  using ts32_t = uint32_t;

  namespace time {

    /**
     * Returns current UNIX timestamp.
     * Represents number of milliseconds since epoch.
     */
    inline auto now() {
      return std::chrono::system_clock::now().time_since_epoch()
          / std::chrono::milliseconds(1);
    }

    /**
     * Return UNIX timestamp with given offset.
     * Represents number of milliseconds since epoch.
     */
    template <typename T>
    inline auto now(const T &offset) {
      return (std::chrono::system_clock::now().time_since_epoch() + offset)
          / std::chrono::milliseconds(1);
    }

    using time_t = decltype(now());
  }  // namespace time
}  // namespace kagome

#endif  // KAGOME_CORE_COMMON_TIME_HPP
