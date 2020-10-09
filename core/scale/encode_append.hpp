/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_ENCODE_APPEND_HPP
#define KAGOME_CORE_SCALE_ENCODE_APPEND_HPP

#include "scale/scale.hpp"

namespace kagome::scale {

  /**
   * Vector wrapper, that is scale encoded without prepended CompactInteger
   */
  struct EncodeOpaqueValue {
    gsl::span<const uint8_t> v;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const EncodeOpaqueValue &value) {
    for (auto &&it = value.v.begin(), end = value.v.end(); it != end; ++it) {
      s << *it;
    }
    return s;
  }

  /**
   * Adds to scale encoded vector of EncodeOpaqueValue another
   * EncodeOpaqueValue. If current vector is empty, then it is replaced by new
   * EncodeOpaqueValue
   * In other words what actually happens could be implemented like that:
   * @code{.cpp}
   * auto vec = scale::decode<vector<EncodeOpaqueValue>>(self_encoded);
   * vec.push_back(scale::encode(EncodeOpaqueValue(input));
   * self_encoded = scale::encode(vec);
   * @endcode
   * but actual implementation a bit more optimal
   * @param self_encoded Current encoded vector of EncodeOpaqueValue
   * @param input is a vector, that is encoded as EncodeOpaqueValue and added to
   * @param self_encoded
   * @return success input was appended to self_encoded, failure otherwise
   */
  outcome::result<void> append_or_new_vec(std::vector<uint8_t> &self_encoded,
                                          gsl::span<const uint8_t> input);
}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_ENCODE_APPEND_HPP
