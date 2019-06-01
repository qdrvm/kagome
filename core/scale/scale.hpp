/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_HPP
#define KAGOME_SCALE_HPP

#include <vector>

#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <gsl/span>
#include <outcome/outcome.hpp>
#include "scale/scale_decoder_stream.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  /**
   * @brief convenience function for encoding primitives data to stream
   * @tparam T primitive type
   * @param t data to encode
   * @return encoded data
   */
  template <class T>
  outcome::result<std::vector<uint8_t>> encode(T &&t) {
    ScaleEncoderStream s;
    OUTCOME_CATCH((s << t))
    return s.data();
  }

  /**
   * @brief convenience function for decoding primitives data from stream
   * @param span
   * @return
   */
  template <class T>
  outcome::result<T> decode(gsl::span<const uint8_t> span) {
    T t{};
    ScaleDecoderStream s(span);
    OUTCOME_CATCH((s >> t))
    return t;
  }

  /**
   * @brief when you already have a stream to decode from it
   * @tparam T value type
   * @param s stream reference
   * @return decoded value or error
   */
  template <class T>
  outcome::result<T> decode(ScaleDecoderStream &s) {
    T t{};
    OUTCOME_CATCH((s >> t))
    return t;
  }
}  // namespace kagome::scale
#endif  // KAGOME_SCALE_HPP
