/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HASHER_HASHER_HPP_
#define KAGOME_STREAM_HASHER_HASHER_HPP_

#include <gsl/span>
#include "crypto/stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename... T>
  inline void hashTypes(T &&...t, StreamHasher &hasher, gsl::span<uint8_t> out) {
    constexpr size_t kBufferSize = 256;
    uint8_t buffer[kBufferSize];
    size_t counter = 0ull;

    encode(
        [&](const uint8_t *const val, size_t count) {
          const auto remains = kBufferSize - counter;
          if (remains >= count) {
            memcpy(&buffer[counter], val, count);
            counter += count;
          } else {
            memcpy(&buffer[counter], val, count);
          }
        },
        std::forward<T>(t)...);
  }

  //  struct StreamHasher {
  //    using Hash64 = common::Hash64;
  //    using Hash128 = common::Hash128;
  //    using Hash256 = common::Hash256;
  //    using Hash512 = common::Hash512;
  //    virtual ~StreamHasher() = default;
  //
  //    /**
  //     * @brief updates hash context internal state with new data
  //     * @param buffer source value
  //     */
  //    virtual bool update(gsl::span<const uint8_t> buffer) = 0;
  //
  //    /**
  //     * @brief store the result of the hashing into outlined buffer
  //     * @param out buffer vector
  //     */
  //    virtual bool get_final(gsl::span<uint8_t> out) = 0;
  //  };
}  // namespace kagome::crypto

#endif  // KAGOME_STREAM_HASHER_HASHER_HPP_
