/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPE_HASHER_HASHER_HPP_
#define KAGOME_TYPE_HASHER_HASHER_HPP_

#include <gsl/span>
#include "crypto/hasher/blake2b_stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename H, typename... T>
  inline void hashTypes(H &hasher, gsl::span<uint8_t> out, T &&...t) {
    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          hasher.update({val, (ssize_t)count});
        },
        std::forward<T>(t)...);

    hasher.get_final(out);
  }

}  // namespace kagome::crypto

#endif  // KAGOME_TYPE_HASHER_HASHER_HPP_
