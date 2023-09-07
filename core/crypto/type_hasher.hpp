/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPE_HASHER_HASHER_HPP_
#define KAGOME_TYPE_HASHER_HASHER_HPP_

#include <gsl/span>
#include "crypto/stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename... T>
  inline void hashTypes(StreamHasher &hasher, gsl::span<uint8_t> out, T &&...t) {
    constexpr size_t kBufferSize = 128;
    uint8_t buffer[kBufferSize];
    size_t counter = 0ull;

    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          if (count >= kBufferSize) {
            hasher.update({buffer, (ssize_t)counter});
            counter = 0ull;
            hasher.update({val, (ssize_t)count});
            return;
          }
          
          while (count != 0ull) {
            const auto to_copy = std::min(kBufferSize - counter, count);
            memcpy(&buffer[counter], val, to_copy);

            counter += to_copy;
            count -= to_copy;

            if (counter == kBufferSize) {
              hasher.update({buffer, (ssize_t)counter});
              counter = 0ull;
            }
          }
        },
        std::forward<T>(t)...);

    if (counter != 0ull) {
      hasher.update({buffer, (ssize_t)counter});
    }

    hasher.get_final(out);
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

#endif  // KAGOME_TYPE_HASHER_HASHER_HPP_
