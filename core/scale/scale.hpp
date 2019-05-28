/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_HPP
#define KAGOME_SCALE_HPP

#include <vector>

#include <gsl/span>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <outcome/outcome.hpp>
#include "scale/scale_encoder_stream.hpp"
#include "scale/scale_decoder_stream.hpp"

namespace kagome::scale {
/**
 * @brief convenience function for encoding primitives data to stream
 * @tparam T primitive type
 * @param t data to encode
 * @return encoded data
 */
  template <class T>
  outcome::result<std::vector<uint8_t>> scaleEncode(T &&t) {
    try {
      ScaleEncoderStream s;
      s << t;
      return s.data();
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  /**
   * @brief convenience function for decoding primitives data from stream
   * @param span
   * @return
   */
  template <class T>
  outcome::result<T> scaleDecode(gsl::span<const uint8_t> span) {
    try {
      T t;
      ScaleDecoderStream s(span);
      s >> t;
      return t;
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }
}  // namespace kagome::scale
#endif  // KAGOME_SCALE_HPP
